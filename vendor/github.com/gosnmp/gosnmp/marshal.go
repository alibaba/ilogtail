// Copyright 2012 The GoSNMP Authors. All rights reserved.  Use of this
// source code is governed by a BSD-style license that can be found in the
// LICENSE file.

package gosnmp

import (
	"bytes"
	"context"
	"encoding/asn1"
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"net"
	"runtime"
	"strings"
	"sync/atomic"
	"time"
)

//
// Remaining globals and definitions located here.
// See http://www.rane.com/note161.html for a succint description of the SNMP
// protocol.
//

// SnmpVersion 1, 2c and 3 implemented
type SnmpVersion uint8

// SnmpVersion 1, 2c and 3 implemented
const (
	Version1  SnmpVersion = 0x0
	Version2c SnmpVersion = 0x1
	Version3  SnmpVersion = 0x3
)

// SnmpPacket struct represents the entire SNMP Message or Sequence at the
// application layer.
type SnmpPacket struct {
	Version            SnmpVersion
	MsgFlags           SnmpV3MsgFlags
	SecurityModel      SnmpV3SecurityModel
	SecurityParameters SnmpV3SecurityParameters // interface
	ContextEngineID    string
	ContextName        string
	Community          string
	PDUType            PDUType
	MsgID              uint32
	RequestID          uint32
	MsgMaxSize         uint32
	Error              SNMPError
	ErrorIndex         uint8
	NonRepeaters       uint8
	MaxRepetitions     uint32
	Variables          []SnmpPDU
	Logger             Logger // interface

	// v1 traps have a very different format from v2c and v3 traps.
	//
	// These fields are set via the SnmpTrap parameter to SendTrap().
	SnmpTrap
}

// SnmpTrap is used to define a SNMP trap, and is passed into SendTrap
type SnmpTrap struct {
	Variables []SnmpPDU

	// If true, the trap is an InformRequest, not a trap. This has no effect on
	// v1 traps, as Inform is not part of the v1 protocol.
	IsInform bool

	// These fields are required for SNMPV1 Trap Headers
	Enterprise   string
	AgentAddress string
	GenericTrap  int
	SpecificTrap int
	Timestamp    uint
}

// VarBind struct represents an SNMP Varbind.
type VarBind struct {
	Name  asn1.ObjectIdentifier
	Value asn1.RawValue
}

// PDUType describes which SNMP Protocol Data Unit is being sent.
type PDUType byte

// The currently supported PDUType's
const (
	Sequence       PDUType = 0x30
	GetRequest     PDUType = 0xa0
	GetNextRequest PDUType = 0xa1
	GetResponse    PDUType = 0xa2
	SetRequest     PDUType = 0xa3
	Trap           PDUType = 0xa4 // v1
	GetBulkRequest PDUType = 0xa5
	InformRequest  PDUType = 0xa6
	SNMPv2Trap     PDUType = 0xa7 // v2c, v3
	Report         PDUType = 0xa8 // v3
)

// SNMPv3: User-based Security Model Report PDUs and
// error types as per https://tools.ietf.org/html/rfc3414
const (
	usmStatsUnsupportedSecLevels = ".1.3.6.1.6.3.15.1.1.1.0"
	usmStatsNotInTimeWindows     = ".1.3.6.1.6.3.15.1.1.2.0"
	usmStatsUnknownUserNames     = ".1.3.6.1.6.3.15.1.1.3.0"
	usmStatsUnknownEngineIDs     = ".1.3.6.1.6.3.15.1.1.4.0"
	usmStatsWrongDigests         = ".1.3.6.1.6.3.15.1.1.5.0"
	usmStatsDecryptionErrors     = ".1.3.6.1.6.3.15.1.1.6.0"
)

var (
	ErrDecryption           = errors.New("decryption error")
	ErrNotInTimeWindow      = errors.New("not in time window")
	ErrUnknownEngineID      = errors.New("unknown engine id")
	ErrUnknownReportPDU     = errors.New("unknown report pdu")
	ErrUnknownSecurityLevel = errors.New("unknown security level")
	ErrUnknownUsername      = errors.New("unknown username")
	ErrWrongDigest          = errors.New("wrong digest")
)

const rxBufSize = 65535 // max size of IPv4 & IPv6 packet

// Logger is an interface used for debugging. Both Print and
// Printf have the same interfaces as Package Log in the std library. The
// Logger interface is small to give you flexibility in how you do
// your debugging.
//
// For verbose logging to stdout:
//
//     gosnmp_logger = log.New(os.Stdout, "", 0)
type Logger interface {
	Print(v ...interface{})
	Printf(format string, v ...interface{})
}

func (x *GoSNMP) logPrint(v ...interface{}) {
	if x.loggingEnabled {
		x.Logger.Print(v...)
	}
}

func (x *GoSNMP) logPrintf(format string, v ...interface{}) {
	if x.loggingEnabled {
		x.Logger.Printf(format, v...)
	}
}

// send/receive one snmp request
func (x *GoSNMP) sendOneRequest(packetOut *SnmpPacket,
	wait bool) (result *SnmpPacket, err error) {
	allReqIDs := make([]uint32, 0, x.Retries+1)
	// allMsgIDs := make([]uint32, 0, x.Retries+1) // unused

	timeout := x.Timeout
	withContextDeadline := false
	for retries := 0; ; retries++ {
		if retries > 0 {
			if x.OnRetry != nil {
				x.OnRetry(x)
			}

			x.logPrintf("Retry number %d. Last error was: %v", retries, err)
			if withContextDeadline && strings.Contains(err.Error(), "timeout") {
				err = context.DeadlineExceeded
				break
			}
			if retries > x.Retries {
				if strings.Contains(err.Error(), "timeout") {
					err = fmt.Errorf("request timeout (after %d retries)", retries-1)
				}
				break
			}
			if x.ExponentialTimeout {
				// https://www.webnms.com/snmp/help/snmpapi/snmpv3/v1/timeout.html
				timeout *= 2
			}
			withContextDeadline = false
		}
		err = nil

		if x.Context.Err() != nil {
			return nil, x.Context.Err()
		}

		reqDeadline := time.Now().Add(timeout)
		if contextDeadline, ok := x.Context.Deadline(); ok {
			if contextDeadline.Before(reqDeadline) {
				reqDeadline = contextDeadline
				withContextDeadline = true
			}
		}

		err = x.Conn.SetDeadline(reqDeadline)
		if err != nil {
			return nil, err
		}

		// Request ID is an atomic counter that wraps to 0 at max int32.
		reqID := (atomic.AddUint32(&(x.requestID), 1) & 0x7FFFFFFF)
		allReqIDs = append(allReqIDs, reqID)

		packetOut.RequestID = reqID

		if x.Version == Version3 {
			msgID := (atomic.AddUint32(&(x.msgID), 1) & 0x7FFFFFFF)

			// allMsgIDs = append(allMsgIDs, msgID) // unused

			packetOut.MsgID = msgID

			err = x.initPacket(packetOut)
			if err != nil {
				break
			}
		}
		if x.loggingEnabled && x.Version == Version3 {
			packetOut.SecurityParameters.Log()
		}

		var outBuf []byte
		outBuf, err = packetOut.marshalMsg()
		if err != nil {
			// Don't retry - not going to get any better!
			err = fmt.Errorf("marshal: %w", err)
			break
		}

		if x.PreSend != nil {
			x.PreSend(x)
		}
		x.logPrintf("SENDING PACKET: %#+v", *packetOut)
		// If using UDP and unconnected socket, send packet directly to stored address.
		if uconn, ok := x.Conn.(net.PacketConn); ok && x.uaddr != nil {
			_, err = uconn.WriteTo(outBuf, x.uaddr)
		} else {
			_, err = x.Conn.Write(outBuf)
		}
		if err != nil {
			continue
		}
		if x.OnSent != nil {
			x.OnSent(x)
		}

		// all sends wait for the return packet, except for SNMPv2Trap
		if !wait {
			return &SnmpPacket{}, nil
		}

	waitingResponse:
		for {
			x.logPrint("WAITING RESPONSE...")
			// Receive response and try receiving again on any decoding error.
			// Let the deadline abort us if we don't receive a valid response.

			var resp []byte
			resp, err = x.receive()
			if err == io.EOF && strings.HasPrefix(x.Transport, "tcp") {
				// EOF on TCP: reconnect and retry. Do not count
				// as retry as socket was broken
				x.logPrintf("ERROR: EOF. Performing reconnect")
				err = x.netConnect()
				if err != nil {
					return nil, err
				}
				retries--
				break
			} else if err != nil {
				// receive error. retrying won't help. abort
				break
			}
			if x.OnRecv != nil {
				x.OnRecv(x)
			}
			x.logPrintf("GET RESPONSE OK: %+v", resp)
			result = new(SnmpPacket)
			result.Logger = x.Logger

			result.MsgFlags = packetOut.MsgFlags
			if packetOut.SecurityParameters != nil {
				result.SecurityParameters = packetOut.SecurityParameters.Copy()
			}

			var cursor int
			cursor, err = x.unmarshalHeader(resp, result)
			if err != nil {
				x.logPrintf("ERROR on unmarshall header: %s", err)
				continue
			}

			if x.Version == Version3 {
				useResponseSecurityParameters := false
				if usp, ok := x.SecurityParameters.(*UsmSecurityParameters); ok {
					if usp.AuthoritativeEngineID == "" {
						useResponseSecurityParameters = true
					}
				}
				err = x.testAuthentication(resp, result, useResponseSecurityParameters)
				if err != nil {
					x.logPrintf("ERROR on Test Authentication on v3: %s", err)
					break
				}
				resp, cursor, _ = x.decryptPacket(resp, cursor, result)
			}

			err = x.unmarshalPayload(resp, cursor, result)
			if err != nil {
				x.logPrintf("ERROR on UnmarshalPayload on v3: %s", err)
				continue
			}
			if result.Error == NoError && len(result.Variables) < 1 {
				x.logPrintf("ERROR on UnmarshalPayload on v3: Empty result")
				continue
			}

			// While Report PDU was defined by RFC 1905 as part of SNMPv2, it was never
			// used until SNMPv3. Report PDU's allow a SNMP engine to tell another SNMP
			// engine that an error was detected while processing an SNMP message.
			//
			// The format for a Report PDU is
			// -----------------------------------
			// | 0xA8 | reqid | 0 | 0 | varbinds |
			// -----------------------------------
			// where:
			// - PDU type 0xA8 indicates a Report PDU.
			// - reqid is either:
			//    The request identifier of the message that triggered the report
			//    or zero if the request identifier cannot be extracted.
			// - The variable bindings will contain a single object identifier and its value
			//
			// usmStatsNotInTimeWindows and usmStatsUnknownEngineIDs are recoverable errors
			// and will be retransmitted, for others we return the result with an error.
			if result.Version == Version3 && result.PDUType == Report && len(result.Variables) == 1 {
				switch result.Variables[0].Name {
				case usmStatsUnsupportedSecLevels:
					return result, ErrUnknownSecurityLevel
				case usmStatsNotInTimeWindows:
					break waitingResponse
				case usmStatsUnknownUserNames:
					return result, ErrUnknownUsername
				case usmStatsUnknownEngineIDs:
					break waitingResponse
				case usmStatsWrongDigests:
					return result, ErrWrongDigest
				case usmStatsDecryptionErrors:
					return result, ErrDecryption
				default:
					return result, ErrUnknownReportPDU
				}
			}

			validID := false
			for _, id := range allReqIDs {
				if id == result.RequestID {
					validID = true
				}
			}
			if result.RequestID == 0 {
				validID = true
			}
			if !validID {
				x.logPrint("ERROR  out of order")
				continue
			}

			break
		}
		if err != nil {
			continue
		}

		if x.OnFinish != nil {
			x.OnFinish(x)
		}
		// Success!
		return result, nil
	}

	// Return last error
	return nil, err
}

// generic "sender" that negotiate any version of snmp request
//
// all sends wait for the return packet, except for SNMPv2Trap
func (x *GoSNMP) send(packetOut *SnmpPacket, wait bool) (result *SnmpPacket, err error) {
	defer func() {
		if e := recover(); e != nil {
			var buf = make([]byte, 8192)
			runtime.Stack(buf, true)

			err = fmt.Errorf("recover: %v Stack:%v", e, string(buf))
		}
	}()

	if x.Conn == nil {
		return nil, fmt.Errorf("&GoSNMP.Conn is missing. Provide a connection or use Connect()")
	}

	if x.Retries < 0 {
		x.Retries = 0
	}
	x.logPrint("SEND INIT")
	if packetOut.Version == Version3 {
		x.logPrint("SEND INIT NEGOTIATE SECURITY PARAMS")
		if err = x.negotiateInitialSecurityParameters(packetOut); err != nil {
			return &SnmpPacket{}, err
		}
		x.logPrint("SEND END NEGOTIATE SECURITY PARAMS")
	}

	// perform request
	result, err = x.sendOneRequest(packetOut, wait)
	if err != nil {
		x.logPrintf("SEND Error on the first Request Error: %s", err)
		return result, err
	}

	if result.Version == Version3 {
		x.logPrintf("SEND STORE SECURITY PARAMS from result: %+v", result)
		err = x.storeSecurityParameters(result)

		if result.PDUType == Report && len(result.Variables) == 1 {
			switch result.Variables[0].Name {
			case usmStatsNotInTimeWindows:
				x.logPrint("WARNING detected out-of-time-window ERROR")
				if err = x.updatePktSecurityParameters(packetOut); err != nil {
					x.logPrintf("ERROR  updatePktSecurityParameters error: %s", err)
					return nil, err
				}
				// retransmit with updated auth engine params
				result, err = x.sendOneRequest(packetOut, wait)
				if err != nil {
					x.logPrintf("ERROR  out-of-time-window retransmit error: %s", err)
					return result, ErrNotInTimeWindow
				}

			case usmStatsUnknownEngineIDs:
				x.logPrint("WARNING detected unknown engine id ERROR")
				if err = x.updatePktSecurityParameters(packetOut); err != nil {
					x.logPrintf("ERROR  updatePktSecurityParameters error: %s", err)
					return nil, err
				}
				// retransmit with updated engine id
				result, err = x.sendOneRequest(packetOut, wait)
				if err != nil {
					x.logPrintf("ERROR unknown engine id retransmit error: %s", err)
					return result, ErrUnknownEngineID
				}
			}
		}
	}
	return result, err
}
func (packet *SnmpPacket) logPrintf(format string, v ...interface{}) {
	if packet.Logger != nil {
		packet.Logger.Printf(format, v...)
	}
}

// -- Marshalling Logic --------------------------------------------------------

// MarshalMsg marshalls a snmp packet, ready for sending across the wire
func (packet *SnmpPacket) MarshalMsg() ([]byte, error) {
	return packet.marshalMsg()
}

// marshal an SNMP message
func (packet *SnmpPacket) marshalMsg() ([]byte, error) {
	var err error
	buf := new(bytes.Buffer)

	// version
	buf.Write([]byte{2, 1, byte(packet.Version)})

	if packet.Version == Version3 {
		buf, err = packet.marshalV3(buf)
		if err != nil {
			return nil, err
		}
	} else {
		// community
		buf.Write([]byte{4, uint8(len(packet.Community))})
		buf.WriteString(packet.Community)
		// pdu
		pdu, err2 := packet.marshalPDU()
		if err2 != nil {
			return nil, err2
		}
		buf.Write(pdu)
	}

	// build up resulting msg - sequence, length then the tail (buf)
	msg := new(bytes.Buffer)
	msg.WriteByte(byte(Sequence))

	bufLengthBytes, err2 := marshalLength(buf.Len())
	if err2 != nil {
		return nil, err2
	}
	msg.Write(bufLengthBytes)
	_, err = buf.WriteTo(msg)
	if err != nil {
		return nil, err
	}

	authenticatedMessage, err := packet.authenticate(msg.Bytes())
	if err != nil {
		return nil, err
	}

	return authenticatedMessage, nil
}

func (packet *SnmpPacket) marshalSNMPV1TrapHeader() ([]byte, error) {
	buf := new(bytes.Buffer)

	// marshal OID
	oidBytes, err := marshalOID(packet.Enterprise)
	if err != nil {
		return nil, fmt.Errorf("unable to marshal OID: %w", err)
	}
	buf.Write([]byte{byte(ObjectIdentifier), byte(len(oidBytes))})
	buf.Write(oidBytes)

	// marshal AgentAddress (ip address)
	ip := net.ParseIP(packet.AgentAddress)
	ipAddressBytes := ipv4toBytes(ip)
	buf.Write([]byte{byte(IPAddress), byte(len(ipAddressBytes))})
	buf.Write(ipAddressBytes)

	// marshal GenericTrap. Could just cast GenericTrap to a single byte as IDs greater than 6 are unknown,
	// but do it properly. See issue 182.
	var genericTrapBytes []byte
	genericTrapBytes, err = marshalInt32(packet.GenericTrap)
	if err != nil {
		return nil, fmt.Errorf("unable to marshal SNMPv1 GenericTrap: %w", err)
	}
	buf.Write([]byte{byte(Integer), byte(len(genericTrapBytes))})
	buf.Write(genericTrapBytes)

	// marshal SpecificTrap
	var specificTrapBytes []byte
	specificTrapBytes, err = marshalInt32(packet.SpecificTrap)
	if err != nil {
		return nil, fmt.Errorf("unable to marshal SNMPv1 SpecificTrap: %w", err)
	}
	buf.Write([]byte{byte(Integer), byte(len(specificTrapBytes))})
	buf.Write(specificTrapBytes)

	// marshal timeTicks
	timeTickBytes, err := marshalUint32(uint32(packet.Timestamp))
	if err != nil {
		return nil, fmt.Errorf("unable to Timestamp: %w", err)
	}
	buf.Write([]byte{byte(TimeTicks), byte(len(timeTickBytes))})
	buf.Write(timeTickBytes)

	return buf.Bytes(), nil
}

// marshal a PDU
func (packet *SnmpPacket) marshalPDU() ([]byte, error) {
	buf := new(bytes.Buffer)

	switch packet.PDUType {
	case GetBulkRequest:
		// requestid
		buf.Write([]byte{2, 4})
		err := binary.Write(buf, binary.BigEndian, packet.RequestID)
		if err != nil {
			return nil, err
		}

		// non repeaters
		nonRepeaters, err := marshalUint32(packet.NonRepeaters)
		if err != nil {
			return nil, fmt.Errorf("marshalPDU: unable to marshal NonRepeaters to uint32: %w", err)
		}

		buf.Write([]byte{2, byte(len(nonRepeaters))})
		if err = binary.Write(buf, binary.BigEndian, nonRepeaters); err != nil {
			return nil, fmt.Errorf("marshalPDU: unable to marshal NonRepeaters: %w", err)
		}

		// max repetitions
		maxRepetitions, err := marshalUint32(packet.MaxRepetitions)
		if err != nil {
			return nil, fmt.Errorf("marshalPDU: unable to marshal maxRepetitions to uint32: %w", err)
		}

		buf.Write([]byte{2, byte(len(maxRepetitions))})
		if err = binary.Write(buf, binary.BigEndian, maxRepetitions); err != nil {
			return nil, fmt.Errorf("marshalPDU: unable to marshal maxRepetitions: %w", err)
		}

	case Trap:
		// write SNMP V1 Trap Header fields
		snmpV1TrapHeader, err := packet.marshalSNMPV1TrapHeader()
		if err != nil {
			return nil, err
		}

		buf.Write(snmpV1TrapHeader)

	default:
		// requestid
		buf.Write([]byte{2, 4})
		err := binary.Write(buf, binary.BigEndian, packet.RequestID)
		if err != nil {
			return nil, fmt.Errorf("unable to marshal OID: %w", err)
		}

		// error status
		errorStatus, err := marshalUint32(uint8(packet.Error))
		if err != nil {
			return nil, fmt.Errorf("marshalPDU: unable to marshal errorStatus to uint32: %w", err)
		}

		buf.Write([]byte{2, byte(len(errorStatus))})
		if err = binary.Write(buf, binary.BigEndian, errorStatus); err != nil {
			return nil, fmt.Errorf("marshalPDU: unable to marshal errorStatus: %w", err)
		}

		// error index
		errorIndex, err := marshalUint32(packet.ErrorIndex)
		if err != nil {
			return nil, fmt.Errorf("marshalPDU: unable to marshal errorIndex to uint32: %w", err)
		}

		buf.Write([]byte{2, byte(len(errorIndex))})
		if err = binary.Write(buf, binary.BigEndian, errorIndex); err != nil {
			return nil, fmt.Errorf("marshalPDU: unable to marshal errorIndex: %w", err)
		}
	}

	// build varbind list
	vbl, err := packet.marshalVBL()
	if err != nil {
		return nil, fmt.Errorf("marshalPDU: unable to marshal varbind list: %w", err)
	}
	buf.Write(vbl)

	// build up resulting pdu
	pdu := new(bytes.Buffer)
	// calculate pdu length
	bufLengthBytes, err := marshalLength(buf.Len())
	if err != nil {
		return nil, fmt.Errorf("marshalPDU: unable to marshal pdu length: %w", err)
	}
	// write request type
	pdu.WriteByte(byte(packet.PDUType))
	// write pdu length
	pdu.Write(bufLengthBytes)
	// write the tail (buf)
	if _, err = buf.WriteTo(pdu); err != nil {
		return nil, fmt.Errorf("marshalPDU: unable to marshal pdu: %w", err)
	}

	return pdu.Bytes(), nil
}

// marshal a varbind list
func (packet *SnmpPacket) marshalVBL() ([]byte, error) {
	vblBuf := new(bytes.Buffer)
	for _, pdu := range packet.Variables {
		pdu := pdu
		vb, err := marshalVarbind(&pdu)
		if err != nil {
			return nil, err
		}
		vblBuf.Write(vb)
	}

	vblBytes := vblBuf.Bytes()
	vblLengthBytes, err := marshalLength(len(vblBytes))
	if err != nil {
		return nil, err
	}

	// FIX does bytes.Buffer give better performance than byte slices?
	result := []byte{byte(Sequence)}
	result = append(result, vblLengthBytes...)
	result = append(result, vblBytes...)
	return result, nil
}

// marshal a varbind
func marshalVarbind(pdu *SnmpPDU) ([]byte, error) {
	oid, err := marshalOID(pdu.Name)
	if err != nil {
		return nil, err
	}
	pduBuf := new(bytes.Buffer)
	tmpBuf := new(bytes.Buffer)

	// Marshal the PDU type into the appropriate BER
	switch pdu.Type {
	case Null:
		ltmp, err2 := marshalLength(len(oid))
		if err2 != nil {
			return nil, err2
		}
		tmpBuf.Write([]byte{byte(ObjectIdentifier)})
		tmpBuf.Write(ltmp)
		tmpBuf.Write(oid)
		tmpBuf.Write([]byte{byte(Null), byte(EndOfContents)})

		ltmp, err2 = marshalLength(tmpBuf.Len())
		if err2 != nil {
			return nil, err2
		}
		pduBuf.Write([]byte{byte(Sequence)})
		pduBuf.Write(ltmp)
		_, err2 = tmpBuf.WriteTo(pduBuf)
		if err2 != nil {
			return nil, err2
		}

	case Integer:
		// Oid
		tmpBuf.Write([]byte{byte(ObjectIdentifier), byte(len(oid))})
		tmpBuf.Write(oid)

		// Number
		var intBytes []byte
		switch value := pdu.Value.(type) {
		case byte:
			intBytes = []byte{byte(pdu.Value.(int))}
		case int:
			if intBytes, err = marshalInt32(value); err != nil {
				return nil, fmt.Errorf("error mashalling PDU Integer: %w", err)
			}
		default:
			return nil, fmt.Errorf("unable to marshal PDU Integer; not byte or int")
		}
		tmpBuf.Write([]byte{byte(Integer), byte(len(intBytes))})
		tmpBuf.Write(intBytes)

		// Sequence, length of oid + integer, then oid/integer data
		pduBuf.WriteByte(byte(Sequence))
		pduBuf.WriteByte(byte(len(oid) + len(intBytes) + 4))
		pduBuf.Write(tmpBuf.Bytes())

	case Counter32, Gauge32, TimeTicks, Uinteger32:
		// Oid
		tmpBuf.Write([]byte{byte(ObjectIdentifier), byte(len(oid))})
		tmpBuf.Write(oid)

		// Number
		var intBytes []byte
		switch value := pdu.Value.(type) {
		case uint32:
			if intBytes, err = marshalUint32(value); err != nil {
				return nil, fmt.Errorf("error marshalling PDU Uinteger32 type from uint32: %w", err)
			}
		case uint:
			if intBytes, err = marshalUint32(uint32(value)); err != nil {
				return nil, fmt.Errorf("error marshalling PDU Uinteger32 type from uint: %w", err)
			}
		default:
			return nil, fmt.Errorf("unable to marshal pdu.Type %v; unknown pdu.Value %v[type=%T]", pdu.Type, pdu.Value, pdu.Value)
		}
		tmpBuf.Write([]byte{byte(pdu.Type), byte(len(intBytes))})
		tmpBuf.Write(intBytes)

		// Sequence, length of oid + integer, then oid/integer data
		pduBuf.WriteByte(byte(Sequence))
		pduBuf.WriteByte(byte(len(oid) + len(intBytes) + 4))
		pduBuf.Write(tmpBuf.Bytes())

	case OctetString, BitString:
		// Oid
		tmpBuf.Write([]byte{byte(ObjectIdentifier), byte(len(oid))})
		tmpBuf.Write(oid)

		// OctetString
		var octetStringBytes []byte
		switch value := pdu.Value.(type) {
		case []byte:
			octetStringBytes = value
		case string:
			octetStringBytes = []byte(value)
		default:
			return nil, fmt.Errorf("unable to marshal PDU OctetString; not []byte or string")
		}

		var length []byte
		length, err = marshalLength(len(octetStringBytes))
		if err != nil {
			return nil, fmt.Errorf("unable to marshal PDU length: %w", err)
		}
		tmpBuf.WriteByte(byte(pdu.Type))
		tmpBuf.Write(length)
		tmpBuf.Write(octetStringBytes)

		tmpBytes := tmpBuf.Bytes()

		length, err = marshalLength(len(tmpBytes))
		if err != nil {
			return nil, fmt.Errorf("unable to marshal PDU data length: %w", err)
		}
		// Sequence, length of oid + octetstring, then oid/octetstring data
		pduBuf.WriteByte(byte(Sequence))

		pduBuf.Write(length)
		pduBuf.Write(tmpBytes)

	case ObjectIdentifier:
		// Oid
		tmpBuf.Write([]byte{byte(ObjectIdentifier), byte(len(oid))})
		tmpBuf.Write(oid)
		value := pdu.Value.(string)
		oidBytes, err := marshalOID(value)
		if err != nil {
			return nil, fmt.Errorf("error marshalling ObjectIdentifier: %w", err)
		}

		// Oid data
		var length []byte
		length, err = marshalLength(len(oidBytes))
		if err != nil {
			return nil, fmt.Errorf("error marshalling ObjectIdentifier length: %w", err)
		}
		tmpBuf.WriteByte(byte(pdu.Type))
		tmpBuf.Write(length)
		tmpBuf.Write(oidBytes)

		tmpBytes := tmpBuf.Bytes()
		length, err = marshalLength(len(tmpBytes))
		if err != nil {
			return nil, fmt.Errorf("error marshalling ObjectIdentifier data length: %w", err)
		}
		// Sequence, length of oid + oid, then oid/oid data
		pduBuf.WriteByte(byte(Sequence))
		pduBuf.Write(length)
		pduBuf.Write(tmpBytes)

	case IPAddress:
		// Oid
		tmpBuf.Write([]byte{byte(ObjectIdentifier), byte(len(oid))})
		tmpBuf.Write(oid)
		// OctetString
		var ipAddressBytes []byte
		switch value := pdu.Value.(type) {
		case []byte:
			ipAddressBytes = value
		case string:
			ip := net.ParseIP(value)
			ipAddressBytes = ipv4toBytes(ip)
		default:
			return nil, fmt.Errorf("unable to marshal PDU IPAddress; not []byte or string")
		}
		tmpBuf.Write([]byte{byte(IPAddress), byte(len(ipAddressBytes))})
		tmpBuf.Write(ipAddressBytes)
		// Sequence, length of oid + octetstring, then oid/octetstring data
		pduBuf.WriteByte(byte(Sequence))
		pduBuf.WriteByte(byte(len(oid) + len(ipAddressBytes) + 4))
		pduBuf.Write(tmpBuf.Bytes())
	case Counter64, OpaqueFloat, OpaqueDouble:
		converters := map[Asn1BER]func(interface{}) ([]byte, error){
			Counter64:    marshalUint64,
			OpaqueFloat:  marshalFloat32,
			OpaqueDouble: marshalFloat64,
		}
		tmpBuf.Write([]byte{byte(ObjectIdentifier), byte(len(oid))})
		tmpBuf.Write(oid)
		tmpBuf.WriteByte(byte(pdu.Type))
		intBytes, err := converters[pdu.Type](pdu.Value)
		if err != nil {
			return nil, fmt.Errorf("error converting PDU value type %v to %v: %w", pdu.Value, pdu.Type, err)
		}

		tmpBuf.WriteByte(byte(len(intBytes)))
		tmpBuf.Write(intBytes)
		tmpBytes := tmpBuf.Bytes()
		length, err := marshalLength(len(tmpBytes))
		if err != nil {
			return nil, fmt.Errorf("error marshalling Float type length: %w", err)
		}
		// Sequence, length of oid + oid, then oid/oid data
		pduBuf.WriteByte(byte(Sequence))
		pduBuf.Write(length)
		pduBuf.Write(tmpBytes)
	case NoSuchInstance, NoSuchObject, EndOfMibView:
		tmpBuf.Write([]byte{byte(ObjectIdentifier), byte(len(oid))})
		tmpBuf.Write(oid)
		tmpBuf.WriteByte(byte(pdu.Type))
		tmpBuf.WriteByte(byte(EndOfContents))
		tmpBytes := tmpBuf.Bytes()
		length, err := marshalLength(len(tmpBytes))
		if err != nil {
			return nil, fmt.Errorf("error marshalling Null type data length: %w", err)
		}
		// Sequence, length of oid + oid, then oid/oid data
		pduBuf.WriteByte(byte(Sequence))
		pduBuf.Write(length)
		pduBuf.Write(tmpBytes)
	default:
		return nil, fmt.Errorf("unable to marshal PDU: unknown BER type %q", pdu.Type)
	}

	return pduBuf.Bytes(), nil
}

// -- Unmarshalling Logic ------------------------------------------------------

func (x *GoSNMP) unmarshalHeader(packet []byte, response *SnmpPacket) (int, error) {
	if len(packet) < 2 {
		return 0, fmt.Errorf("cannot unmarshal empty packet")
	}
	if response == nil {
		return 0, fmt.Errorf("cannot unmarshal response into nil packet reference")
	}

	response.Variables = make([]SnmpPDU, 0, 5)

	// Start parsing the packet
	cursor := 0

	// First bytes should be 0x30
	if PDUType(packet[0]) != Sequence {
		return 0, fmt.Errorf("invalid packet header")
	}

	length, cursor := parseLength(packet)
	if len(packet) != length {
		return 0, fmt.Errorf("error verifying packet sanity: Got %d Expected: %d", len(packet), length)
	}
	x.logPrintf("Packet sanity verified, we got all the bytes (%d)", length)

	// Parse SNMP Version
	rawVersion, count, err := parseRawField(x.Logger, packet[cursor:], "version")
	if err != nil {
		return 0, fmt.Errorf("error parsing SNMP packet version: %w", err)
	}

	cursor += count
	if cursor > len(packet) {
		return 0, fmt.Errorf("error parsing SNMP packet, packet length %d cursor %d", len(packet), cursor)
	}

	if version, ok := rawVersion.(int); ok {
		response.Version = SnmpVersion(version)
		x.logPrintf("Parsed version %d", version)
	}

	if response.Version == Version3 {
		oldcursor := cursor
		cursor, err = x.unmarshalV3Header(packet, cursor, response)
		if err != nil {
			return 0, err
		}
		x.logPrintf("UnmarshalV3Header done. [with SecurityParameters]. Header Size %d. Last 4 Bytes=[%v]", cursor-oldcursor, packet[cursor-4:cursor])
	} else {
		// Parse community
		rawCommunity, count, err := parseRawField(x.Logger, packet[cursor:], "community")
		if err != nil {
			return 0, fmt.Errorf("error parsing community string: %w", err)
		}
		cursor += count
		if cursor > len(packet) {
			return 0, fmt.Errorf("error parsing SNMP packet, packet length %d cursor %d", len(packet), cursor)
		}

		if community, ok := rawCommunity.(string); ok {
			response.Community = community
			x.logPrintf("Parsed community %s", community)
		}
	}
	return cursor, nil
}

func (x *GoSNMP) unmarshalPayload(packet []byte, cursor int, response *SnmpPacket) error {
	var err error
	// Parse SNMP packet type
	requestType := PDUType(packet[cursor])
	x.logPrintf("UnmarshalPayload Meet PDUType %#x. Offset %v", requestType, cursor)
	switch requestType {
	// known, supported types
	case GetResponse, GetNextRequest, GetBulkRequest, Report, SNMPv2Trap, GetRequest, SetRequest, InformRequest:
		response.PDUType = requestType
		err = x.unmarshalResponse(packet[cursor:], response)
		if err != nil {
			return fmt.Errorf("error in unmarshalResponse: %w", err)
		}
		// If it's an InformRequest, mark the trap.
		response.IsInform = (requestType == InformRequest)
	case Trap:
		response.PDUType = requestType
		err = x.unmarshalTrapV1(packet[cursor:], response)
		if err != nil {
			return fmt.Errorf("error in unmarshalTrapV1: %w", err)
		}
	default:
		x.logPrintf("UnmarshalPayload Meet Unknown PDUType %#x. Offset %v", requestType, cursor)
		return fmt.Errorf("unknown PDUType %#x", requestType)
	}
	return nil
}

func (x *GoSNMP) unmarshalResponse(packet []byte, response *SnmpPacket) error {
	cursor := 0

	getResponseLength, cursor := parseLength(packet)
	if len(packet) != getResponseLength {
		return fmt.Errorf("error verifying Response sanity: Got %d Expected: %d", len(packet), getResponseLength)
	}
	x.logPrintf("getResponseLength: %d", getResponseLength)

	// Parse Request-ID
	rawRequestID, count, err := parseRawField(x.Logger, packet[cursor:], "request id")
	if err != nil {
		return fmt.Errorf("error parsing SNMP packet request ID: %w", err)
	}
	cursor += count
	if cursor > len(packet) {
		return fmt.Errorf("error parsing SNMP packet, packet length %d cursor %d", len(packet), cursor)
	}

	if requestid, ok := rawRequestID.(int); ok {
		response.RequestID = uint32(requestid)
		x.logPrintf("requestID: %d", response.RequestID)
	}

	if response.PDUType == GetBulkRequest {
		// Parse Non Repeaters
		rawNonRepeaters, count, err := parseRawField(x.Logger, packet[cursor:], "non repeaters")
		if err != nil {
			return fmt.Errorf("error parsing SNMP packet non repeaters: %w", err)
		}
		cursor += count
		if cursor > len(packet) {
			return fmt.Errorf("error parsing SNMP packet, packet length %d cursor %d", len(packet), cursor)
		}

		if nonRepeaters, ok := rawNonRepeaters.(int); ok {
			response.NonRepeaters = uint8(nonRepeaters)
		}

		// Parse Max Repetitions
		rawMaxRepetitions, count, err := parseRawField(x.Logger, packet[cursor:], "max repetitions")
		if err != nil {
			return fmt.Errorf("error parsing SNMP packet max repetitions: %w", err)
		}
		cursor += count
		if cursor > len(packet) {
			return fmt.Errorf("error parsing SNMP packet, packet length %d cursor %d", len(packet), cursor)
		}

		if maxRepetitions, ok := rawMaxRepetitions.(uint32); ok {
			response.MaxRepetitions = (maxRepetitions & 0x7FFFFFFF)
		}
	} else {
		// Parse Error-Status
		rawError, count, err := parseRawField(x.Logger, packet[cursor:], "error-status")
		if err != nil {
			return fmt.Errorf("error parsing SNMP packet error: %w", err)
		}
		cursor += count
		if cursor > len(packet) {
			return fmt.Errorf("error parsing SNMP packet, packet length %d cursor %d", len(packet), cursor)
		}

		if errorStatus, ok := rawError.(int); ok {
			response.Error = SNMPError(errorStatus)
			x.logPrintf("errorStatus: %d", uint8(errorStatus))
		}

		// Parse Error-Index
		rawErrorIndex, count, err := parseRawField(x.Logger, packet[cursor:], "error index")
		if err != nil {
			return fmt.Errorf("error parsing SNMP packet error index: %w", err)
		}
		cursor += count
		if cursor > len(packet) {
			return fmt.Errorf("error parsing SNMP packet, packet length %d cursor %d", len(packet), cursor)
		}

		if errorindex, ok := rawErrorIndex.(int); ok {
			response.ErrorIndex = uint8(errorindex)
			x.logPrintf("error-index: %d", uint8(errorindex))
		}
	}

	return x.unmarshalVBL(packet[cursor:], response)
}

func (x *GoSNMP) unmarshalTrapV1(packet []byte, response *SnmpPacket) error {
	cursor := 0

	getResponseLength, cursor := parseLength(packet)
	if len(packet) != getResponseLength {
		return fmt.Errorf("error verifying Response sanity: Got %d Expected: %d", len(packet), getResponseLength)
	}
	x.logPrintf("getResponseLength: %d", getResponseLength)

	// Parse Enterprise
	rawEnterprise, count, err := parseRawField(x.Logger, packet[cursor:], "enterprise")
	if err != nil {
		return fmt.Errorf("error parsing SNMP packet error: %w", err)
	}
	cursor += count
	if cursor > len(packet) {
		return fmt.Errorf("error parsing SNMP packet, packet length %d cursor %d", len(packet), cursor)
	}

	if Enterprise, ok := rawEnterprise.([]int); ok {
		response.Enterprise = oidToString(Enterprise)
		x.logPrintf("Enterprise: %+v", Enterprise)
	}

	// Parse AgentAddress
	rawAgentAddress, count, err := parseRawField(x.Logger, packet[cursor:], "agent-address")
	if err != nil {
		return fmt.Errorf("error parsing SNMP packet error: %w", err)
	}
	cursor += count
	if cursor > len(packet) {
		return fmt.Errorf("error parsing SNMP packet, packet length %d cursor %d", len(packet), cursor)
	}

	if AgentAddress, ok := rawAgentAddress.(string); ok {
		response.AgentAddress = AgentAddress
		x.logPrintf("AgentAddress: %s", AgentAddress)
	}

	// Parse GenericTrap
	rawGenericTrap, count, err := parseRawField(x.Logger, packet[cursor:], "generic-trap")
	if err != nil {
		return fmt.Errorf("error parsing SNMP packet error: %w", err)
	}
	cursor += count
	if cursor > len(packet) {
		return fmt.Errorf("error parsing SNMP packet, packet length %d cursor %d", len(packet), cursor)
	}

	if GenericTrap, ok := rawGenericTrap.(int); ok {
		response.GenericTrap = GenericTrap
		x.logPrintf("GenericTrap: %d", GenericTrap)
	}

	// Parse SpecificTrap
	rawSpecificTrap, count, err := parseRawField(x.Logger, packet[cursor:], "specific-trap")
	if err != nil {
		return fmt.Errorf("error parsing SNMP packet error: %w", err)
	}
	cursor += count
	if cursor > len(packet) {
		return fmt.Errorf("error parsing SNMP packet, packet length %d cursor %d", len(packet), cursor)
	}

	if SpecificTrap, ok := rawSpecificTrap.(int); ok {
		response.SpecificTrap = SpecificTrap
		x.logPrintf("SpecificTrap: %d", SpecificTrap)
	}

	// Parse TimeStamp
	rawTimestamp, count, err := parseRawField(x.Logger, packet[cursor:], "time-stamp")
	if err != nil {
		return fmt.Errorf("error parsing SNMP packet error: %w", err)
	}
	cursor += count
	if cursor > len(packet) {
		return fmt.Errorf("error parsing SNMP packet, packet length %d cursor %d", len(packet), cursor)
	}

	if Timestamp, ok := rawTimestamp.(uint); ok {
		response.Timestamp = Timestamp
		x.logPrintf("Timestamp: %d", Timestamp)
	}

	return x.unmarshalVBL(packet[cursor:], response)
}

// unmarshal a Varbind list
func (x *GoSNMP) unmarshalVBL(packet []byte, response *SnmpPacket) error {
	var cursor, cursorInc int
	var vblLength int

	if len(packet) == 0 || cursor > len(packet) {
		return fmt.Errorf("truncated packet when unmarshalling a VBL, got length %d cursor %d", len(packet), cursor)
	}

	if packet[cursor] != 0x30 {
		return fmt.Errorf("expected a sequence when unmarshalling a VBL, got %x", packet[cursor])
	}

	vblLength, cursor = parseLength(packet)
	if vblLength == 0 || vblLength > len(packet) {
		return fmt.Errorf("truncated packet when unmarshalling a VBL, packet length %d cursor %d", len(packet), cursor)
	}

	if len(packet) != vblLength {
		return fmt.Errorf("error verifying: packet length %d vbl length %d", len(packet), vblLength)
	}
	x.logPrintf("vblLength: %d", vblLength)

	// check for an empty response
	if vblLength == 2 && packet[1] == 0x00 {
		return nil
	}

	// Loop & parse Varbinds
	for cursor < vblLength {
		if packet[cursor] != 0x30 {
			return fmt.Errorf("expected a sequence when unmarshalling a VB, got %x", packet[cursor])
		}

		_, cursorInc = parseLength(packet[cursor:])
		cursor += cursorInc
		if cursor > len(packet) {
			return fmt.Errorf("error parsing OID Value: packet %d cursor %d", len(packet), cursor)
		}

		// Parse OID
		rawOid, oidLength, err := parseRawField(x.Logger, packet[cursor:], "OID")
		if err != nil {
			return fmt.Errorf("error parsing OID Value: %w", err)
		}

		cursor += oidLength
		if cursor > len(packet) {
			return fmt.Errorf("error parsing OID Value: truncated, packet length %d cursor %d", len(packet), cursor)
		}

		var oid []int
		var ok bool
		if oid, ok = rawOid.([]int); !ok {
			return fmt.Errorf("unable to type assert rawOid |%v| to []int", rawOid)
		}
		oidStr := oidToString(oid)
		x.logPrintf("OID: %s", oidStr)

		// Parse Value
		v, err := x.decodeValue(packet[cursor:], "value")
		if err != nil {
			return fmt.Errorf("error decoding value: %w", err)
		}

		valueLength, _ := parseLength(packet[cursor:])
		cursor += valueLength
		if cursor > len(packet) {
			return fmt.Errorf("error decoding OID Value: truncated, packet length %d cursor %d", len(packet), cursor)
		}

		response.Variables = append(response.Variables, SnmpPDU{oidStr, v.Type, v.Value})
	}
	return nil
}

// receive response from network and read into a byte array
func (x *GoSNMP) receive() ([]byte, error) {
	var n int
	var err error
	// If we are using UDP and unconnected socket, read the packet and
	// disregard the source address.
	if uconn, ok := x.Conn.(net.PacketConn); ok {
		n, _, err = uconn.ReadFrom(x.rxBuf[:])
	} else {
		n, err = x.Conn.Read(x.rxBuf[:])
	}
	if err == io.EOF {
		return nil, err
	} else if err != nil {
		return nil, fmt.Errorf("error reading from socket: %w", err)
	}

	if n == rxBufSize {
		// This should never happen unless we're using something like a unix domain socket.
		return nil, fmt.Errorf("response buffer too small")
	}

	resp := make([]byte, n)
	copy(resp, x.rxBuf[:n])
	return resp, nil
}
