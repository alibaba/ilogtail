# Copyright 2024 iLogtail Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.22)
project(prometheus_unittest)

add_executable(prometheus_input_runner_unittest PrometheusInputRunnerUnittest.cpp)
add_executable(textparser_unittest TextParserUnittest.cpp)
add_executable(scrape_work_unittest ScrapeWorkUnittest.cpp)
add_executable(scraper_unittest ScraperUnittest.cpp)
add_executable(labels_unittest LabelsUnittest.cpp)
add_executable(relabel_unittest RelabelUnittest.cpp)

target_link_libraries(prometheus_input_runner_unittest unittest_base)
target_link_libraries(textparser_unittest unittest_base)
target_link_libraries(scrape_work_unittest unittest_base)
target_link_libraries(scraper_unittest unittest_base)
target_link_libraries(labels_unittest unittest_base)
target_link_libraries(relabel_unittest unittest_base)

include(GoogleTest)

gtest_discover_tests(prometheus_input_runner_unittest)
gtest_discover_tests(textparser_unittest)
gtest_discover_tests(scrape_work_unittest)
gtest_discover_tests(scraper_unittest)
gtest_discover_tests(labels_unittest)
gtest_discover_tests(relabel_unittest)
