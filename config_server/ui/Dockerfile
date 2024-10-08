FROM anolis-registry.cn-zhangjiakou.cr.aliyuncs.com/openanolis/node:18.17.1-23 AS builder

USER root
WORKDIR /frontend

COPY . .

RUN npm install  \
    && npm run build

FROM nginx:alpine

WORKDIR /usr/share/nginx/html
RUN rm -rf ./*

COPY --from=builder /frontend/dist .

COPY --from=builder /frontend/nginx/default.conf /etc/nginx/conf.d/default.conf

CMD ["nginx", "-g", "daemon off;"]
