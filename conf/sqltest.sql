SELECT
 AcctUniqueId,
 UserName,
 NASIPAddress,
 NASPort,
 NASPortType,
 AcctStartTime,
 AcctStopTime,
 AcctSessionTime,
 AcctAuthentic,
 AcctInputOctets,
 AcctOutputOctets,
 CalledStationId,
 CallingStationId,
 AcctTerminateCause,
 AcctStartDelay,
 AcctStopDelay
FROM radacct;
INSERT into radacct (
 AcctSessionId,
 AcctUniqueId,
 UserName,
 Realm,
 NASIPAddress,
 NASPort,
 NASPortType,
 AcctStartTime,
 AcctSessionTime,
 AcctAuthentic,
 ConnectInfo_start,
 ConnectInfo_stop,
 AcctInputOctets,
 AcctOutputOctets,
 CalledStationId,
 CallingStationId,
 AcctTerminateCause,
 ServiceType,
 FramedProtocol,
 FramedIPAddress,
 AcctStartDelay,
 AcctStopDelay
) values (
 '10.241.110.220 test 03/01/93 00:41:43 00000002',
 'b59098c5aa7890',
 'tester',
 '',
 '10.241.110.220',
 '50022',
 'Ethernet',
 '2007-04-29 17:58:30',
 '110',
 'RADIUS',
 '',
 '',
 '8764',
 '58399',
 '00-17-95-93-A4-D6',
 '00-03-47-3D-9F-2C',
 'Port-Error',
 '',
 '',
 '',
 '20',
 ''
);
CREATE TABLE radacct (
  RadAcctId BIGINT NOT NULL,
  AcctSessionId VARCHAR(256) NOT NULL DEFAULT '1234',
  AcctUniqueId VARCHAR(32) NOT NULL DEFAULT '',
  UserName VARCHAR(64) NOT NULL DEFAULT '',
  Realm VARCHAR(64) DEFAULT '',
  NASIPAddress VARCHAR(15) NOT NULL DEFAULT '',
  NASPort VARCHAR(15) DEFAULT NULL,
  NASPortType VARCHAR(32) DEFAULT NULL,
  AcctStartTime datetime NOT NULL DEFAULT 0,
  AcctStopTime datetime NOT NULL DEFAULT 0,
  AcctSessionTime INTEGER DEFAULT NULL,
  AcctAuthentic VARCHAR(32) DEFAULT NULL,
  ConnectInfo_start VARCHAR(50) DEFAULT NULL,
  ConnectInfo_stop VARCHAR(50) DEFAULT NULL,
  AcctInputOctets BIGINT DEFAULT NULL,
  AcctOutputOctets BIGINT DEFAULT NULL,
  CalledStationId VARCHAR(50) NOT NULL DEFAULT '',
  CallingStationId VARCHAR(50) NOT NULL DEFAULT '',
  AcctTerminateCause VARCHAR(32) NOT NULL DEFAULT '',
  ServiceType VARCHAR(32) DEFAULT NULL,
  FramedProtocol VARCHAR(32) DEFAULT NULL,
  FramedIPAddress VARCHAR(15) NOT NULL DEFAULT '',
  AcctStartDelay INTEGER DEFAULT NULL CHECK (AcctStartDelay > 1),
  AcctStopDelay INTEGER DEFAULT NULL,
  UNIQUE (RadAcctId),
  PRIMARY KEY ( RadAcctId )
);
UPDATE radacct SET
 AcctStopTime='2007-04-29 19:58:30.000',
 AcctSessionTime = '280',
 AcctInputOctets = '4590',
 AcctOutputOctets = '7980',
 AcctTerminateCause = 'Port-Error',
 AcctStopDelay = '20',
 ConnectInfo_stop = ''
WHERE
 AcctSessionId = '10.241.110.220 test 03/01/93 00:41:43 00000002' AND
 UserName = 'tester' AND
 NASIPAddress = '10.241.110.220' AND
 AcctStopTime = 0;

DROP TABLE radacct CASCADE;

DELETE FROM radacct
WHERE
UserName = 'tester'
AND AcctStopTime < 2;

