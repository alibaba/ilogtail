// Copyright 2023 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package aesencrypt

import (
	"testing"
)

func TestAes_encrypt(t *testing.T) {
	key := "0000000000000000000000000000000000000000000000000000000000000000"
	// iv := "00000000000000000000000000000000"
	aesEncrypt := &AesEncrypt{}
	if err := aesEncrypt.Init("aes_encrypt", key); err != nil {
		t.Errorf("Failed to initialize aes_encrypt: %v", err)
	}

	// Test case 1: encrypt a string
	plaintext := "0123456"
	expectedCiphertext := "bc3acdbd40c283d91f7dc7010fd7d2b1"
	ciphertext, err := aesEncrypt.Process(plaintext, key)
	if err != nil {
		t.Errorf("Failed to encrypt plaintext: %v", err)
	}
	if ciphertext != expectedCiphertext {
		t.Errorf("Encryption failed. Expected: %v, got: %v", expectedCiphertext, ciphertext)
	}

	// // Test case 2: encrypt an empty string
	// plaintext = ""
	// expectedCiphertext = "d5d5d5d5d5d5d5d5d5d5d5d5d5d5d5d5"
	// ciphertext, err = aesEncrypt.Process(plaintext, key)
	// if err != nil {
	// 	t.Errorf("Failed to encrypt plaintext: %v", err)
	// }
	// if ciphertext != expectedCiphertext {
	// 	t.Errorf("Encryption failed. Expected: %v, got: %v", expectedCiphertext, ciphertext)
	// }

	// // Test case 3: encrypt a string with a null character
	// plaintext = "hello\x00world"
	// expectedCiphertext = "f7d1f3d4c8f5d8e5d8f5d8e5d8f5d8e5"
	// ciphertext, err = aesEncrypt.Process(plaintext, key)
	// if err != nil {
	// 	t.Errorf("Failed to encrypt plaintext: %v", err)
	// }
	// if ciphertext != expectedCiphertext {
	// 	t.Errorf("Encryption failed. Expected: %v, got: %v", expectedCiphertext, ciphertext)
	// }

	// Test case 4: encrypt a null string
	plaintext = "NULL"
	expectedCiphertext = "NULL"
	ciphertext, err = aesEncrypt.Process(plaintext, key)
	if err != nil {
		t.Errorf("Failed to encrypt plaintext: %v", err)
	}
	if ciphertext != expectedCiphertext {
		t.Errorf("Encryption failed. Expected: %v, got: %v", expectedCiphertext, ciphertext)
	}

	// Test case 5: encrypt with incorrect number of parameters
	_, err = aesEncrypt.Process("hello")
	if err == nil {
		t.Errorf("Expected error for incorrect number of parameters")
	}
}
