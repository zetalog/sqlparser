/***************************************************************************
 * db_mssql.sql                 					   *
 *                                                                         *
 * Database schema for MSSQL server					   *
 *									   *
 * To load:								   *
 *  isql -S db_ip_addr -d db_name -U db_login -P db_passwd -i db_mssql.sql *
 *									   *
 * Based on: db_mysql.sql (Mike Machado <mike@innercite.com>)		   *
 *									   *
 *					Dmitri Ageev <d_ageev@ortcc.ru>    *
 ***************************************************************************/

/****** Object:  Table [radacct]    Script Date: 26.03.02 16:55:17 ******/
CREATE TABLE [radacct] (
	[RadAcctId] [numeric](21, 0) IDENTITY (1, 1) NOT NULL ,
	[AcctSessionId] [varchar] (32) NOT NULL ,
	[AcctUniqueId] [varchar] (32) NOT NULL ,
	[UserName] [varchar] (64) NOT NULL ,
	[Realm] [varchar] (64) NULL ,
	[NASIPAddress] [varchar] (15) NOT NULL ,
	[NASPortId] [varchar] (15) NULL ,
	[NASPortType] [varchar] (32) NULL ,
	[AcctStartTime] [datetime] NOT NULL ,
	[AcctStopTime] [datetime] NOT NULL ,
	[AcctSessionTime] [int] NULL ,
	[AcctAuthentic] [varchar] (32) NULL ,
	[ConnectInfo_start] [varchar] (32) NULL ,
	[ConnectInfo_stop] [varchar] (32) NULL ,
	[AcctInputOctets] [int] NULL ,
	[AcctOutputOctets] [int] NULL ,
	[CalledStationId] [varchar] (30) NOT NULL ,
	[CallingStationId] [varchar] (30) NOT NULL ,
	[AcctTerminateCause] [varchar] (32) NOT NULL ,
	[ServiceType] [varchar] (32) NULL ,
	[FramedProtocol] [varchar] (32) NULL ,
	[FramedIPAddress] [varchar] (15) NOT NULL ,
	[AcctStartDelay] [int] NULL ,
	[AcctStopDelay] [int] NULL
) ON [PRIMARY]
GO

/****** Object:  Table [radcheck]    Script Date: 26.03.02 16:55:17 ******/
CREATE TABLE [radcheck] (
	[id] [int] IDENTITY (1, 1) NOT NULL ,
	[UserName] [varchar] (64) NOT NULL ,
	[Attribute] [varchar] (32) NOT NULL ,
	[Value] [varchar] (253) NOT NULL ,
	[op] [char] (2) NULL
) ON [PRIMARY]
GO

/****** Object:  Table [radgroupcheck]    Script Date: 26.03.02 16:55:17 ******/
CREATE TABLE [radgroupcheck] (
	[id] [int] IDENTITY (1, 1) NOT NULL ,
	[GroupName] [varchar] (64) NOT NULL ,
	[Attribute] [varchar] (32) NOT NULL ,
	[Value] [varchar] (253) NOT NULL ,
	[op] [char] (2) NULL
) ON [PRIMARY]
GO

/****** Object:  Table [radgroupreply]    Script Date: 26.03.02 16:55:17 ******/
CREATE TABLE [radgroupreply] (
	[id] [int] IDENTITY (1, 1) NOT NULL ,
	[GroupName] [varchar] (64) NOT NULL ,
	[Attribute] [varchar] (32) NOT NULL ,
	[Value] [varchar] (253) NOT NULL ,
	[op] [char] (2) NULL ,
	[prio] [int] NOT NULL
) ON [PRIMARY]
GO

/****** Object:  Table [radreply]    Script Date: 26.03.02 16:55:18 ******/
CREATE TABLE [radreply] (
	[id] [int] IDENTITY (1, 1) NOT NULL ,
	[UserName] [varchar] (64) NOT NULL ,
	[Attribute] [varchar] (32) NOT NULL ,
	[Value] [varchar] (253) NOT NULL ,
	[op] [char] (2) NULL
) ON [PRIMARY]
GO

/****** Object:  Table [usergroup]    Script Date: 26.03.02 16:55:18 ******/
CREATE TABLE [usergroup] (
	[id] [int] IDENTITY (1, 1) NOT NULL ,
	[UserName] [varchar] (64) NOT NULL ,
	[GroupName] [varchar] (64) NULL
) ON [PRIMARY]
GO

ALTER TABLE [radacct] WITH NOCHECK ADD
	CONSTRAINT [DF_radacct_AcctSessionId] DEFAULT ('') FOR [AcctSessionId],
	CONSTRAINT [DF_radacct_AcctUniqueId] DEFAULT ('') FOR [AcctUniqueId],
	CONSTRAINT [DF_radacct_UserName] DEFAULT ('') FOR [UserName],
	CONSTRAINT [DF_radacct_Realm] DEFAULT ('') FOR [Realm],
	CONSTRAINT [DF_radacct_NASIPAddress] DEFAULT ('') FOR [NASIPAddress],
	CONSTRAINT [DF_radacct_NASPortId] DEFAULT (null) FOR [NASPortId],
	CONSTRAINT [DF_radacct_NASPortType] DEFAULT (null) FOR [NASPortType],
	CONSTRAINT [DF_radacct_AcctStartTime] DEFAULT ('1900-01-01 00:00:00') FOR [AcctStartTime],
	CONSTRAINT [DF_radacct_AcctStopTime] DEFAULT ('1900-01-01 00:00:00') FOR [AcctStopTime],
	CONSTRAINT [DF_radacct_AcctSessionTime] DEFAULT (null) FOR [AcctSessionTime],
	CONSTRAINT [DF_radacct_AcctAuthentic] DEFAULT (null) FOR [AcctAuthentic],
	CONSTRAINT [DF_radacct_ConnectInfo_start] DEFAULT (null) FOR [ConnectInfo_start],
	CONSTRAINT [DF_radacct_ConnectInfo_stop] DEFAULT (null) FOR [ConnectInfo_stop],
	CONSTRAINT [DF_radacct_AcctInputOctets] DEFAULT (null) FOR [AcctInputOctets],
	CONSTRAINT [DF_radacct_AcctOutputOctets] DEFAULT (null) FOR [AcctOutputOctets],
	CONSTRAINT [DF_radacct_CalledStationId] DEFAULT ('') FOR [CalledStationId],
	CONSTRAINT [DF_radacct_CallingStationId] DEFAULT ('') FOR [CallingStationId],
	CONSTRAINT [DF_radacct_AcctTerminateCause] DEFAULT ('') FOR [AcctTerminateCause],
	CONSTRAINT [DF_radacct_ServiceType] DEFAULT (null) FOR [ServiceType],
	CONSTRAINT [DF_radacct_FramedProtocol] DEFAULT (null) FOR [FramedProtocol],
	CONSTRAINT [DF_radacct_FramedIPAddress] DEFAULT ('') FOR [FramedIPAddress],
	CONSTRAINT [DF_radacct_AcctStartDelay] DEFAULT (null) FOR [AcctStartDelay],
	CONSTRAINT [DF_radacct_AcctStopDelay] DEFAULT (null) FOR [AcctStopDelay],
	CONSTRAINT [PK_radacct] PRIMARY KEY  NONCLUSTERED
	(
		[RadAcctId]
	)  ON [PRIMARY]
GO

ALTER TABLE [radcheck] WITH NOCHECK ADD
	CONSTRAINT [DF_radcheck_UserName] DEFAULT ('') FOR [UserName],
	CONSTRAINT [DF_radcheck_Attribute] DEFAULT ('') FOR [Attribute],
	CONSTRAINT [DF_radcheck_Value] DEFAULT ('') FOR [Value],
	CONSTRAINT [DF_radcheck_op] DEFAULT (null) FOR [op],
	CONSTRAINT [PK_radcheck] PRIMARY KEY  NONCLUSTERED
	(
		[id]
	)  ON [PRIMARY]
GO

ALTER TABLE [radgroupcheck] WITH NOCHECK ADD
	CONSTRAINT [DF_radgroupcheck_GroupName] DEFAULT ('') FOR [GroupName],
	CONSTRAINT [DF_radgroupcheck_Attribute] DEFAULT ('') FOR [Attribute],
	CONSTRAINT [DF_radgroupcheck_Value] DEFAULT ('') FOR [Value],
	CONSTRAINT [DF_radgroupcheck_op] DEFAULT (null) FOR [op],
	CONSTRAINT [PK_radgroupcheck] PRIMARY KEY  NONCLUSTERED
	(
		[id]
	)  ON [PRIMARY]
GO

ALTER TABLE [radgroupreply] WITH NOCHECK ADD
	CONSTRAINT [DF_radgroupreply_GroupName] DEFAULT ('') FOR [GroupName],
	CONSTRAINT [DF_radgroupreply_Attribute] DEFAULT ('') FOR [Attribute],
	CONSTRAINT [DF_radgroupreply_Value] DEFAULT ('') FOR [Value],
	CONSTRAINT [DF_radgroupreply_op] DEFAULT (null) FOR [op],
	CONSTRAINT [DF_radgroupreply_prio] DEFAULT (0) FOR [prio],
	CONSTRAINT [PK_radgroupreply] PRIMARY KEY  NONCLUSTERED
	(
		[id]
	)  ON [PRIMARY]
GO

ALTER TABLE [radreply] WITH NOCHECK ADD
	CONSTRAINT [DF_radreply_UserName] DEFAULT ('') FOR [UserName],
	CONSTRAINT [DF_radreply_Attribute] DEFAULT ('') FOR [Attribute],
	CONSTRAINT [DF_radreply_Value] DEFAULT ('') FOR [Value],
	CONSTRAINT [DF_radreply_op] DEFAULT (null) FOR [op],
	CONSTRAINT [PK_radreply] PRIMARY KEY  NONCLUSTERED
	(
		[id]
	)  ON [PRIMARY]
GO

ALTER TABLE [usergroup] WITH NOCHECK ADD
	CONSTRAINT [DF_usergroup_UserName] DEFAULT ('') FOR [UserName],
	CONSTRAINT [DF_usergroup_GroupName] DEFAULT ('') FOR [GroupName],
	CONSTRAINT [PK_usergroup] PRIMARY KEY  NONCLUSTERED
	(
		[id]
	)  ON [PRIMARY]
GO

 CREATE  INDEX [UserName] ON [radacct]([UserName]) ON [PRIMARY]
GO

 CREATE  INDEX [FramedIPAddress] ON [radacct]([FramedIPAddress]) ON [PRIMARY]
GO

 CREATE  INDEX [AcctSessionId] ON [radacct]([AcctSessionId]) ON [PRIMARY]
GO

 CREATE  INDEX [AcctUniqueId] ON [radacct]([AcctUniqueId]) ON [PRIMARY]
GO

 CREATE  INDEX [AcctStartTime] ON [radacct]([AcctStartTime]) ON [PRIMARY]
GO

 CREATE  INDEX [AcctStopTime] ON [radacct]([AcctStopTime]) ON [PRIMARY]
GO

 CREATE  INDEX [NASIPAddress] ON [radacct]([NASIPAddress]) ON [PRIMARY]
GO

 CREATE  INDEX [UserName] ON [radcheck]([UserName]) ON [PRIMARY]
GO

 CREATE  INDEX [GroupName] ON [radgroupcheck]([GroupName]) ON [PRIMARY]
GO

 CREATE  INDEX [GroupName] ON [radgroupreply]([GroupName]) ON [PRIMARY]
GO

 CREATE  INDEX [UserName] ON [radreply]([UserName]) ON [PRIMARY]
GO

 CREATE  INDEX [UserName] ON [usergroup]([UserName]) ON [PRIMARY]
GO
