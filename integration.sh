#!/bin/bash
touch BackupPJ.service

echo "[Unit]" > BackupPJ.service
echo "Description=BackupPJ service" >> BackupPJ.service
echo "[Service]" >> BackupPJ.service
echo "Type=simple" >> BackupPJ.service
echo "PIDFile=/run/backupd_pj.pid" >> BackupPJ.service
echo "User=$USERNAME" >> BackupPJ.service
echo "Group=$USERNAME" >> BackupPJ.service
echo "WorkingDirectory=$PWD/bin" >> BackupPJ.service

# all logs written to stderr will be written to this file: backupd.errors.log
# check ENABLE_DEBUG_INFO for this purpose! 
echo "ExecStart=/bin/bash -ce \"./backupd 2> ../backupd.errors.log\"" >> BackupPJ.service
echo "[Install]" >> BackupPJ.service
echo "WantedBy=multi-user.target" >> BackupPJ.service

sudo mv BackupPJ.service -t /etc/systemd/system/