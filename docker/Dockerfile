# Copyright 2021 iLogtail Authors
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

FROM sls-registry.cn-beijing.cr.aliyuncs.com/sls-microservices/logtailplugin-base:1.16-goc as build

WORKDIR /src

COPY . .

RUN make cgobuild

FROM sls-registry.cn-beijing.cr.aliyuncs.com/sls-microservices/logtailplugin-base:1.16-goc

ARG HOST_OS=Unknown
ARG VERSION=Unknown

WORKDIR /ilogtail

# build
COPY --from=build /src/bin/libPluginBase.so /ilogtail/
COPY --from=build /src/docker/ilogtail_config.json /ilogtail/
# todo build
COPY --from=build /src/docker/ilogtail /ilogtail/
COPY --from=build /src/pkg/logtail/libPluginAdapter.so /ilogtail/

RUN chmod -R 755 /ilogtail/
RUN mkdir /ilogtail/checkpoint

ENV HOST_OS=$HOST_OS
ENV LOGTAIL_VERSION=$VERSION

EXPOSE 18689

ENTRYPOINT ["/ilogtail/ilogtail"]
