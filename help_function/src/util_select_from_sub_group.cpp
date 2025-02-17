// ====------ util_select_from_sub_group.cpp---------- -*- C++ -* ----===////
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//
// ===----------------------------------------------------------------------===//
#include <sycl/sycl.hpp>
#include <dpct/dpct.hpp>

#define WARP_SIZE 32
#define DATA_NUM 128

template<typename T = int>
void init_data(T* data, int num) {
  for(int i = 0; i < num; i++)
    data[i] = i;
}

template<typename T = int>
bool verify_data(T* data, T* expect, int num, int step = 1) {
  for(int i = 0; i < num; i = i + step) {
    if(data[i] != expect[i]) {
      return false;
    }
  }
  return true;
}

template<typename T = int>
void print_data(T* data, int num) {
  for (int i = 0; i < num; i++) {
    std::cout << data[i] << ", ";
    if((i+1)%32 == 0)
        std::cout << std::endl;
  }
  std::cout << std::endl;
}

void select_from_sub_group1(unsigned int* data, sycl::nd_item<3> item_ct1) {
  int threadid = item_ct1.get_local_id(2) +
                 item_ct1.get_local_id(1) * item_ct1.get_local_range(2) +
                 item_ct1.get_local_id(0) * item_ct1.get_local_range(2) *
                     item_ct1.get_local_range(1) +
                 item_ct1.get_group(2) * item_ct1.get_local_range(2) *
                     item_ct1.get_local_range(1) * item_ct1.get_local_range(0);
  int output = 0;
  output = dpct::select_from_sub_group(item_ct1.get_sub_group(), threadid,
                                       threadid + 1);
  data[threadid] = output;
}

void select_from_sub_group2(unsigned int* data, sycl::nd_item<3> item_ct1) {
  int threadid = item_ct1.get_local_id(2) +
                 item_ct1.get_local_id(1) * item_ct1.get_local_range(2) +
                 item_ct1.get_local_id(0) * item_ct1.get_local_range(2) *
                     item_ct1.get_local_range(1) +
                 item_ct1.get_group(2) * item_ct1.get_local_range(2) *
                     item_ct1.get_local_range(1) * item_ct1.get_local_range(0);
  int output = 0;
  output = dpct::select_from_sub_group(item_ct1.get_sub_group(), threadid,
                                       threadid + 1, 8);
  data[threadid] = output;
}

int main() {

  dpct::device_ext &dev_ct1 = dpct::get_current_device();
  sycl::queue &q_ct1 = dev_ct1.default_queue();
  bool Result = true;
  int* dev_data = nullptr;
  unsigned int *dev_data_u = nullptr;
  sycl::range<3> GridSize(1, 1, 1);
  sycl::range<3> BlockSize(1, 1, 1);
  dev_data = sycl::malloc_shared<int>(DATA_NUM, q_ct1);
  dev_data_u = sycl::malloc_shared<unsigned int>(DATA_NUM, q_ct1);

  GridSize = sycl::range<3>(1, 1, 2);
  BlockSize = sycl::range<3>(1, 2, 32);
  unsigned int expect1[DATA_NUM] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0,
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 32,
    65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 64,
    97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 96
  };
  init_data<unsigned int>(dev_data_u, DATA_NUM);

  q_ct1.parallel_for(
      sycl::nd_range<3>(GridSize * BlockSize, BlockSize), [=
  ](sycl::nd_item<3> item_ct1) [[intel::reqd_sub_group_size(32)]] {
        select_from_sub_group1(dev_data_u, item_ct1);
      });

  dev_ct1.queues_wait_and_throw();
  if(!verify_data<unsigned int>(dev_data_u, expect1, DATA_NUM)) {
    std::cout << "select_from_sub_group1" << " verify failed" << std::endl;
    Result = false;
    std::cout << "expect:" << std::endl;
    print_data<unsigned int>(expect1, DATA_NUM);
    std::cout << "current result:" << std::endl;
    print_data<unsigned int>(dev_data_u, DATA_NUM);
  }

  GridSize = sycl::range<3>(1, 1, 2);
  BlockSize = sycl::range<3>(1, 2, 32);
  unsigned int expect2[DATA_NUM] = {
    1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 8, 17, 18, 19, 20, 21, 22, 23, 16, 25, 26, 27, 28, 29, 30, 31, 24,
    33, 34, 35, 36, 37, 38, 39, 32, 41, 42, 43, 44, 45, 46, 47, 40, 49, 50, 51, 52, 53, 54, 55, 48, 57, 58, 59, 60, 61, 62, 63, 56,
    65, 66, 67, 68, 69, 70, 71, 64, 73, 74, 75, 76, 77, 78, 79, 72, 81, 82, 83, 84, 85, 86, 87, 80, 89, 90, 91, 92, 93, 94, 95, 88,
    97, 98, 99, 100, 101, 102, 103, 96, 105, 106, 107, 108, 109, 110, 111, 104, 113, 114, 115, 116, 117, 118, 119, 112, 121, 122, 123, 124, 125, 126, 127, 120
  };
  init_data<unsigned int>(dev_data_u, DATA_NUM);

  q_ct1.parallel_for(
      sycl::nd_range<3>(GridSize * BlockSize, BlockSize), [=
  ](sycl::nd_item<3> item_ct1) [[intel::reqd_sub_group_size(32)]] {
        select_from_sub_group2(dev_data_u, item_ct1);
      });

  dev_ct1.queues_wait_and_throw();
  if(!verify_data<unsigned int>(dev_data_u, expect2, DATA_NUM)) {
    std::cout << "select_from_sub_group2" << " verify failed" << std::endl;
    Result = false;
    std::cout << "expect:" << std::endl;
    print_data<unsigned int>(expect2, DATA_NUM);
    std::cout << "current result:" << std::endl;
    print_data<unsigned int>(dev_data_u, DATA_NUM);
  }

  if(Result)
    std::cout << "passed" << std::endl;
  else {
    exit(-1);
  }
  return 0;
}