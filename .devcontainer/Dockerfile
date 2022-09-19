FROM sls-registry.cn-beijing.cr.aliyuncs.com/sls-microservices/ilogtail-build-linux-amd64:admin

ARG USERNAME=admin
USER root

# Create the user
COPY .env /tmp/.env
RUN source /tmp/.env && rm /tmp/.env && \
    if getent passwd $USERNAME; then userdel -f $USERNAME; fi && \
    if getent group $GROUPNAME; then groupdel $GROUPNAME; fi && \
    groupadd --gid $GROUP_GID $GROUPNAME && \
    useradd --uid $USER_UID --gid $GROUP_GID -m $USERNAME && \
    echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME && \
    chmod 0440 /etc/sudoers.d/$USERNAME && \
    chown -R $USERNAME:$GROUPNAME /root/go /opt/logtail $(eval echo ~$USERNAME) && \
    chmod 755 $(eval echo ~$USERNAME)

USER $USERNAME
