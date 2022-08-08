#start SQL Server, start the script to create the DB and import the data
/opt/mssql-tools/bin/sqlcmd -S mssql -U sa -P MSsqlpa#1word -d master -i init.sql
touch /tmp/healthy
sleep 100