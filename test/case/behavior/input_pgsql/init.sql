CREATE TABLE IF NOT EXISTS specialalarmtest (
    id BIGSERIAL NOT NULL, 
	time TIMESTAMP NOT NULL, 
	alarmtype varchar(64) NOT NULL, 
	ip varchar(16) NOT NULL, 
	COUNT INT NOT NULL, 
	PRIMARY KEY (id)
);

insert into specialalarmtest (time, alarmtype, ip, count) values(now(), 'NO_ALARM', '10.10.***.***', 0);
insert into specialalarmtest (time, alarmtype, ip, count) values(now(), 'NO_ALARM', '10.10.***.***', 1);
insert into specialalarmtest (time, alarmtype, ip, count) values(now(), 'NO_ALARM', '10.10.***.***', 2);
insert into specialalarmtest (time, alarmtype, ip, count) values(now(), 'NO_ALARM', '10.10.***.***', 3);
insert into specialalarmtest (time, alarmtype, ip, count) values(now(), 'NO_ALARM', '10.10.***.***', 4);
insert into specialalarmtest (time, alarmtype, ip, count) values(now(), 'NO_ALARM', '10.10.***.***', 5);
insert into specialalarmtest (time, alarmtype, ip, count) values(now(), 'NO_ALARM', '10.10.***.***', 6);
insert into specialalarmtest (time, alarmtype, ip, count) values(now(), 'NO_ALARM', '10.10.***.***', 7);
insert into specialalarmtest (time, alarmtype, ip, count) values(now(), 'NO_ALARM', '10.10.***.***', 8);
insert into specialalarmtest (time, alarmtype, ip, count) values(now(), 'NO_ALARM', '10.10.***.***', 9);
insert into specialalarmtest (time, alarmtype, ip, count) values(now(), 'NO_ALARM', '10.10.***.***', 10);
insert into specialalarmtest (time, alarmtype, ip, count) values(now(), 'NO_ALARM', '10.10.***.***', 11);