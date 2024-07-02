# ilogtail Daemon Process

When ilogtail exits unexpectedly, a daemon is needed to ensure its availability.

## Installing ilogtail

Refer to the Quick Start guide for downloading and installing ilogtail: [Quick Start](quick-start.md)

## Linux Environment

1. Create the Systemd Configuration File
   Adjust the `ExecStart` path in the following script according to the ilogtail installation directory.

   ```text
   cat > /etc/systemd/system/ilogtaild.service << EOF
   [Unit]
   Description=ilogtail

   [Service]
   Type=simple
   User=root
   Restart=always
   RestartSec=60
   ExecStart=/usr/local/ilogtail/ilogtail

   [Install]
   WantedBy=multi-user.target
   EOF
   ```

2. Register the Service

   ```bash
   systemctl daemon-reload
   ```

3. Start the Service

   ```bash
   systemctl restart ilogtaild
   ```

4. Check Service Status

   ```bash
   systemctl status ilogtaild
   ```

   After the service starts, you can find ilogtail.LOG in the ilogtail-<version> directory.

5. Stop the Service

   ```bash
   systemctl stop ilogtaild
   ```
