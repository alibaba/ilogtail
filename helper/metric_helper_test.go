// Copyright 2022 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package helper

import (
	"math"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestComposeBucket(t *testing.T) {
	tests := []struct {
		lower      float64
		upper      float64
		isPositive bool
		expected   string
	}{
		{
			lower:      0.5,
			upper:      1000,
			isPositive: true,
			expected:   "(0.5,1000]",
		},
		{
			lower:      -0.5,
			upper:      1000,
			isPositive: true,
			expected:   "(-0.5,1000]",
		},
		{
			lower:      0.5,
			upper:      1000,
			isPositive: false,
			expected:   "[-1000,-0.5)",
		},
		{
			lower:      math.Inf(-1),
			upper:      1000,
			isPositive: true,
			expected:   "(-Inf,1000]",
		},
		{
			lower:      0,
			upper:      math.Inf(1),
			isPositive: false,
			expected:   "[-Inf,-0)",
		},
	}

	for _, test := range tests {
		bucketRange := ComposeBucketFieldName(test.lower, test.upper, test.isPositive)
		assert.Equal(t, test.expected, bucketRange)
	}
}

func TestComputeBucketBoundary(t *testing.T) {
	tests := []struct {
		bucketRange    string
		isPositive     bool
		expectedBucket float64
	}{
		{
			bucketRange:    "(0.5,1000]",
			isPositive:     true,
			expectedBucket: 0.5,
		},
		{
			bucketRange:    "(-0.5,1000]",
			isPositive:     true,
			expectedBucket: -0.5,
		},
		{
			bucketRange:    "[-1000,-0.5)",
			isPositive:     false,
			expectedBucket: -0.5,
		},
		{
			bucketRange:    "(-Inf,1000]",
			isPositive:     true,
			expectedBucket: math.Inf(-1),
		},
		{
			bucketRange:    "[-Inf,-0)",
			isPositive:     false,
			expectedBucket: 0,
		},
	}

	for _, test := range tests {
		bucket, err := ComputeBucketBoundary(test.bucketRange, test.isPositive)
		assert.NoError(t, err)
		assert.Equal(t, test.expectedBucket, bucket)
	}
}

func TestReverseSlice(t *testing.T) {
	var inputEmpty []float64
	ReverseSlice(inputEmpty)
	assert.Equal(t, []float64(nil), inputEmpty)

	inputFloat := []float64{3, 2, 1, 10}
	ReverseSlice(inputFloat)
	assert.Equal(t, []float64{10, 1, 2, 3}, inputFloat)

	inputInt := []int{1, 2, 3}
	ReverseSlice(inputInt)
	assert.Equal(t, []int{3, 2, 1}, inputInt)

}
