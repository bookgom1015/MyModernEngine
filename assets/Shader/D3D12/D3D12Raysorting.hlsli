#ifndef __D3D12RAYSORTING_HLSLI__
#define __D3D12RAYSORTING_HLSLI__

//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#define RAY_DIRECTION_HASH_KEY_BITS_1D 4   // 최대 4비트
// TODO 깊이와 인덱스 해싱은 레이 트레이싱 시간을 개선하지 못해 0비트로 비활성화했다.
// 깊이 키가 2비트일 때 광선 정렬 오버헤드는 약 20%였다.
#define DEPTH_HASH_KEY_BITS 0 
#define INDEX_HASH_KEY_BITS 0  // (4 - DEPTH_HASH_KEY_BITS) 
#define NUM_RAYS RaySorting::RayGroup::Size
#define NUM_THREADS RaySorting::ThreadGroup::Size
#define MIN_VALID_RAY_DEPTH FLT_10BIT_MIN
#define MAX_RAYS 8192

#define KEY_NUM_BITS (DEPTH_HASH_KEY_BITS + 2 * RAY_DIRECTION_HASH_KEY_BITS_1D + INDEX_HASH_KEY_BITS)
#define NUM_KEYS (1 << KEY_NUM_BITS)        // 가장 큰 키는 무효 키로 예약한다.

// INACTIVE_RAY_KEY는 최대 유효 해시 키보다 크고, 16비트와 NUM_KEYS 범위에 들어야 한다.
#define INACTIVE_RAY_KEY (NUM_KEYS - 1)     // 무효 또는 비활성 광선의 키. 이 광선은 정렬 결과의 끝에 모이며 추적에서 제외된다.

#define INACTIVE_RAY_INDEX_BIT 0x4000
#define INACTIVE_RAY_INDEX_BIT_Y 0x80

#define INVALID_16BIT_KEY_BIT 0x8000      // 공유 메모리 항목이 16비트 키 값인지 표시하는 비트.
#define INVALID_RAY_ORIGIN_DEPTH 0

// 비활성 광선의 인덱스 오프셋에는 INACTIVE_RAY_INDEX_BIT가 설정된다.
#define IsActiveRay(RayGroupRayIndexOffset) (!(RayGroupRayIndexOffset.y & INACTIVE_RAY_INDEX_BIT_Y))
#define GetRawRayIndexOffset(RayGroupRayIndexOffset) uint2(RayGroupRayIndexOffset.x, RayGroupRayIndexOffset.y & ~(INACTIVE_RAY_INDEX_BIT_Y))

#endif // __D3D12RAYSORTING_HLSLI__