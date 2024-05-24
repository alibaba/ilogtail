# Field Encryption

## Introduction

The `processor_encrypt` plugin can encrypt specified fields using AES-128, AES-192, or AES-256 algorithms with PKCS7 padding. **Since encrypted text may contain non-printable characters, the plugin will hex-encode it.**

## Version

[Stable](../stability-level.md)

## Parameters

| Parameter | Type, Default Value | Description |
| --- | --- | --- |
| Type | String, No default (Required) | Plugin type, always set to `processor_encrypt`. |
| SourceKeys | String[], No default (Required) | Names of the fields to be encrypted, supports multiple fields. |
| EncryptionParameters | EncryptionParameter, No default (Required) | Specifies encryption parameters. |
| KeepSourceValueIfError | Boolean, `false` | If encryption fails, whether to keep the original value; if not, the field value will be replaced with `ENCRYPT_ERROR`. By default, this is `false` to ensure data safety. |

* EncryptionParameter parameter details

| Parameter | Type, Default Value | Description |
| --- | --- | --- |
| Key | String, No default | Specifies the key (in hexadecimal), required if `KeyFilePath` is not provided. AES-128, AES-192, and AES-256 require keys of 16, 24, and 32 bytes, respectively, so when specifying the key directly, you need to enter 32, 48, or 64 hexadecimal characters. |
| IV | String, Default: `00000000000000000000000000000000` | Specifies the initialization vector (IV, in hexadecimal). If `KeyFilePath` is provided, this can be left empty. The IV size equals the internal block size, which for AES-128, AES-192, and AES-256 is 8, 12, and 16 bytes, respectively, requiring 16, 24, or 32 hexadecimal characters. |
| KeyFilePath | String, No default | Path to a file where encryption parameters are stored. Required if `Key` is not provided. Supports saving the above parameters (Key, IV) in a JSON file, where the values in the file will override the values specified through the above parameters. |

## Examples

### Example 1: Directly Specify Key and Use Default IV

Encrypt the field `important`, assuming the key (Key) is a binary sequence of all zeros (256 bits) and no IV is set, with plugin defaults. Here's the configuration and output:

* Configuration

```Yaml
processors:
  - Type: processor_encrypt
    SourceKeys:
      - "important"
    EncryptionParameters:
      Key: "0000000000000000000000000000000000000000000000000000000000000000"
```

* Input

```Json
"normal_key": "1",
"another_normal_key": "2",
"important": "0123456"
```

* Output (since encrypted content may contain non-printable characters, it will be hex-encoded)

```Json
"normal_key": "1",
"another_normal_key": "2",
"important": "bc3acdbd40c283d91f7dc7010fd7d2b1"
```

* Decryption using openssl

```shell
$ printf "%b" '\xbc\x3a\xcd\xbd\x40\xc2\x83\xd9\x1f\x7d\xc7\x01\x0f\xd7\xd2\xb1' > ciphertext
$ openssl enc -d -aes-256-cbc -iv 00000000000000000000000000000000 \
    -K 0000000000000000000000000000000000000000000000000000000000000000 \
    -in ciphertext -out plaintext
$ cat plaintext
0123456
```

### Example 2: Directly Specify Key and IV

Encrypt the field `important`, assuming the key (Key) is a binary sequence of all zeros (256 bits) and the IV is a binary sequence of all ones (128 bits). Here's the configuration and output:

* Input

```Json
"normal_key": "1",
"another_normal_key": "2",
"important": "0123456"
```

* Configuration (note that Key/IV are both in hexadecimal)

```Yaml
processors:
  - Type: processor_encrypt
    SourceKeys:
      - "important"
    EncryptionParameters:
      Key: "0000000000000000000000000000000000000000000000000000000000000000"
      IV: "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
```

* Output (since encrypted content may contain non-printable characters, it will be hex-encoded)

```Json
"normal_key": "1",
"another_normal_key": "2",
"important": "af6428af62d698be617b82cffd9e109b"
```

* Decryption using openssl

```shell
$ printf "%b" '\xaf\x64\x28\xaf\x62\xd6\x98\xbe\x61\x7b\x82\xcf\xfd\x9e\x10\x9b' > ciphertext
$ openssl enc -d -aes-256-cbc -iv FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF \
    -K 0000000000000000000000000000000000000000000000000000000000000000 \
    -in ciphertext -out plaintext
$ cat plaintext
0123456
```

### Example 3: Specify Key via a File

Encrypt the field `important`, assuming the key (Key) is a binary sequence of all ones (256 bits) and stored in a local file `/home/admin/aes_key` on the machine. The file does not specify an IV, and uses plugin defaults. Here's the configuration and output:

* Preceding condition

On the machine running logtail, create a file `/home/admin/aes_key.json` (JSON format) and store the key in hexadecimal, with the following command:

```shell
printf "{\"Key\": \"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF\"}" > /home/admin/aes_key.json
```

* Input

```Json
"normal_key": "1",
"another_normal_key": "2",
"important": "0123456"
```

* Configuration

```Yaml
processors:
  - Type: processor_encrypt
    SourceKeys:
      - "important"
    EncryptionParameters:
      KeyFilePath: "/home/admin/aes_key.json"
```

* Output

```Json
"normal_key": "1",
"another_normal_key": "2",
"important": "bc3acdbd40c283d91f7dc7010fd7d2b1"
```

* Decryption using openssl

```shell
$ printf "%b" '\xbc\x3a\xcd\xbd\x40\xc2\x83\xd9\x1f\x7d\xc7\x01\x0f\xd7\xd2\xb1' > ciphertext
$ openssl enc -d -aes-256-cbc -iv 00000000000000000000000000000000 \
    -K 0000000000000000000000000000000000000000000000000000000000000000 \
    -in ciphertext -out plaintext
$ cat plaintext
0123456
```
