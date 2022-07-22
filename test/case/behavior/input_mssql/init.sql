IF NOT EXISTS(SELECT * FROM sys.databases WHERE name = 'LogtailTest')
BEGIN
    CREATE DATABASE [LogtailTest]
END
GO
USE [LogtailTest]
GO

IF NOT EXISTS (SELECT * FROM sysobjects WHERE name='LogtailTestTable' and xtype='U')
BEGIN
    CREATE TABLE LogtailTestTable (
        id INT PRIMARY KEY IDENTITY (1, 1),
        name NVARCHAR(50), 
		quantity INT
    )
END
GO

insert into LogtailTestTable (name, quantity) values('banana', 150);
insert into LogtailTestTable (name, quantity) values('banana', 150);
insert into LogtailTestTable (name, quantity) values('banana', 150);
insert into LogtailTestTable (name, quantity) values('banana', 150);
SELECT * FROM LogtailTestTable;
GO