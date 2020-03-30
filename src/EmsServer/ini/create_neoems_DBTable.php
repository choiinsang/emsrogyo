#!/usr/local/php -f
<?php
	function ProcessCheck()
	{
		$m_ExeStr = "ps -ef | grep createIEmsDBTable.php 2>&1";
		$m_ExeResult;
		$m_phpCnt = 0;
		
		exec($m_ExeStr, $m_ExeResult);
		
		//print_r($m_ExeResult);
		
		foreach($m_ExeResult as $value)
		{
			if(stripos($value, "php -f") > 0)
			{
				$m_phpCnt++;
			}	
		}
		
		if($m_phpCnt > 1)
			return 1;
			
		return 0;
	}
	
	$mShowLog = 1;
	
	
	if(ProcessCheck() == 1)
	{
		if($mShowLog)
			echo "Already Exist\n";
		exit;
		return;	
	}	
	
	//-----------------------------------------------
	//FUNCTION: crontab에서 처리할 db를 생성하도록 한다.
	//- DB 생성유무 체크*/
	//"SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA='$dbname[1]' AND TABLE_NAME='mails' AND COLUMN_NAME='db_partitioning' AND COLUMN_NAME='ApplyTaxdb' AND COLLUMN_NAME='db_ip' AND COLLUMN_NAME='document_id' ";
	//-----------------------------------------------
	function checkdbexist($dbname)
	{
		if(($dbname == NULL)||($dbname == "")){
			echo "[ERR]Check DB Name[$dbname]\n" ;
			return false;
		}
		else{
			//echo "[SUCC]Check DB Name[$dbname]\n" ;
			return true;
		}
	}
	
	//-----------------------------------------------
	// Create DB Tables('mails', 'mails_detail')
	//-----------------------------------------------
	function create_tables($ip, $id, $pw, $dbname)
	{
		$qry_create_db = "CREATE DATABASE `$dbname` /*!40100 DEFAULT CHARACTER SET euckr */;";
		
		$qry_create_mails = "CREATE TABLE `mails` ("
  	."`no` int(11) unsigned NOT NULL AUTO_INCREMENT,"
  	."`mailidx` int(10) unsigned NOT NULL,"
  	."`campaign_no` bigint(20) unsigned NOT NULL,"
  	."`to_name` varchar(100) DEFAULT NULL,"
  	."`to_id` varchar(255) NOT NULL DEFAULT '',"
  	."`to_domain` varchar(255) NOT NULL DEFAULT '',"
  	."`send_date` datetime NOT NULL,"
  	."`smtp_step` tinyint(4) NOT NULL DEFAULT '-1',"
  	."`smtp_code` smallint(6) NOT NULL DEFAULT '0',"
  	."`try_cnt` tinyint(4) NOT NULL DEFAULT '0',"
  	."`email` varchar(255) NOT NULL DEFAULT '',"
  	."`wide_str` text,"
  	."PRIMARY KEY (`no`),"
  	."KEY `campaign_no` (`campaign_no`,`smtp_step`,`try_cnt`),"
  	."KEY `campaign_no_2` (`campaign_no`,`send_date`),"
  	."KEY `mailidx` (`mailidx`)"
		.") ENGINE=InnoDB DEFAULT CHARSET=euckr;";

		
		$qry_create_mails_detail = "CREATE TABLE `mails_detail` ("
  	."`no` int(10) unsigned NOT NULL AUTO_INCREMENT,"
  	."`mail_no` int(10) unsigned NOT NULL,"
  	."`try_cnt` tinyint(4) NOT NULL,"
  	."`smtp_step` tinyint(4) NOT NULL,"
  	."`smtp_code` smallint(6) NOT NULL,"
  	."`explain` varchar(200) DEFAULT NULL,"
  	."`result_code` smallint(6) DEFAULT NULL,"
  	."PRIMARY KEY (`no`),"
  	."UNIQUE KEY `mail_no` (`mail_no`,`try_cnt`)"
		.") ENGINE=InnoDB DEFAULT CHARSET=euckr;";

		
		if(($dbname == NULL)||($dbname == "")){
			echo "[ERR]Check DB Name[$dbname]\n" ;
			return false;
		}
		else{
			$dbconn = mysql_connect($ip, $id, $pw);
			//echo "[CONNECT TO][$ip][$id][$pw][$dbname]\n";
			$retval = mysql_query($qry_create_db, $dbconn);
			//echo "retval[CREATE]:$retval\n";
			$query  = "use $dbname";
			$retval = mysql_query($query, $dbconn);
			//echo "retval[USE DB]:$retval\n";
			$retval = mysql_query($qry_create_mails, $dbconn);
			//echo "retval[mails]:$retval\n";
			$retval = mysql_query($qry_create_mails_detail, $dbconn);
			//echo "retval[mails_detail]:$retval\n";
			
			mysql_close($dbconn);		
			//echo "[SUCC]Check DB Name[$dbname]\n" ;
			return true;
		}
	}
	
	//-----------------------------------------------
	// Create DB Tax Tables('mails', 'mails_detail')
	//-----------------------------------------------
	function create_tax_tables($ip, $id, $pw, $dbname)
	{
		$qry_create_db = "CREATE DATABASE `$dbname` /*!40100 DEFAULT CHARACTER SET euckr */;";
		
		$qry_create_mails = "CREATE TABLE `mails` ("
  	."`no` int(11) unsigned NOT NULL AUTO_INCREMENT,"
  	."`mailidx` int(10) unsigned NOT NULL,"
  	."`campaign_no` bigint(20) unsigned NOT NULL,"
  	."`to_name` varchar(100) DEFAULT NULL,"
  	."`to_id` varchar(255) NOT NULL DEFAULT '',"
  	."`to_domain` varchar(255) NOT NULL DEFAULT '',"
  	."`send_date` datetime NOT NULL,"
  	."`smtp_step` tinyint(4) NOT NULL DEFAULT '-1',"
  	."`smtp_code` smallint(6) NOT NULL DEFAULT '0',"
  	."`try_cnt` tinyint(4) NOT NULL DEFAULT '0',"
  	."`email` varchar(255) NOT NULL DEFAULT '',"
  	."`wide_str` text,"
  	."`db_ip` varchar(20) DEFAULT '',"
  	."`db_partitioning` varchar(20) DEFAULT '',"
  	."`document_id` varchar(255) DEFAULT '',"
  	."`ApplyTaxdb` enum('Y','N') NOT NULL DEFAULT 'N',"
  	."PRIMARY KEY (`no`),"
  	."KEY `campaign_no` (`campaign_no`,`smtp_step`,`try_cnt`),"
  	."KEY `campaign_no_2` (`campaign_no`,`send_date`),"
  	."KEY `mailidx` (`mailidx`)"
		.") ENGINE=InnoDB DEFAULT CHARSET=euckr;";
		
		$qry_create_mails_detail = "CREATE TABLE `mails_detail` ("
  	."`no` int(10) unsigned NOT NULL AUTO_INCREMENT,"
  	."`mail_no` int(10) unsigned NOT NULL,"
  	."`try_cnt` tinyint(4) NOT NULL,"
  	."`smtp_step` tinyint(4) NOT NULL,"
  	."`smtp_code` smallint(6) NOT NULL,"
  	."`explain` varchar(200) DEFAULT NULL,"
  	."`result_code` smallint(6) DEFAULT NULL,"
  	."PRIMARY KEY (`no`),"
  	."UNIQUE KEY `mail_no` (`mail_no`,`try_cnt`)"
		.") ENGINE=InnoDB DEFAULT CHARSET=euckr;";

		
		if(($dbname == NULL)||($dbname == "")){
			echo "[ERR]Check DB Name[$dbname]\n" ;
			return false;
		}
		else{
			$dbconn = mysql_connect($ip, $id, $pw);
			//echo "[CONNECT TO][$ip][$id][$pw][$dbname]\n";
			$retval = mysql_query($qry_create_db, $dbconn);
			//echo "retval[CREATE]:$retval\n";
			$query  = "use $dbname";
			$retval = mysql_query($query, $dbconn);
			//echo "retval[USE DB]:$retval\n";
			$retval = mysql_query($qry_create_mails, $dbconn);
			//echo "retval[mails]:$retval\n";
			$retval = mysql_query($qry_create_mails_detail, $dbconn);
			//echo "retval[mails_detail]:$retval\n";
			
			mysql_close($dbconn);		
			//echo "[SUCC]Check DB Name[$dbname]\n" ;
			return true;
		}
	}
	
	//-----------------------------------------------
	// Create DB Service Tables('mails', 'mails_detail')
	//-----------------------------------------------
	function create_service_tables($ip, $id, $pw, $dbname)
	{
		$qry_create_db = "CREATE DATABASE `$dbname` /*!40100 DEFAULT CHARACTER SET euckr */;";
		
		$qry_create_mails = "CREATE TABLE `mails` ("
		."`no` int(11) unsigned NOT NULL AUTO_INCREMENT,"
		."`mailidx` int(10) unsigned NOT NULL,"
		."`campaign_no` bigint(20) unsigned NOT NULL,"
		."`to_name` varchar(100) DEFAULT NULL,"
		."`to_id` varchar(255) NOT NULL DEFAULT '',"
		."`to_domain` varchar(255) NOT NULL DEFAULT '',"
		."`send_date` datetime NOT NULL,"
		."`smtp_step` tinyint(4) NOT NULL DEFAULT '-1',"
		."`smtp_code` smallint(6) NOT NULL DEFAULT '0',"
		."`try_cnt` tinyint(4) NOT NULL DEFAULT '0',"
		."`email` varchar(255) NOT NULL DEFAULT '',"
		."`wide_str` text,"
		."`is_read` enum('Y','N') NOT NULL DEFAULT 'N',"
		."PRIMARY KEY (`no`),"
		."KEY `campaign_no` (`campaign_no`,`smtp_step`,`try_cnt`),"
		."KEY `campaign_no_2` (`campaign_no`,`send_date`),"
		."KEY `mailidx` (`mailidx`)"
		.") ENGINE=InnoDB  DEFAULT CHARSET=euckr;";

		
		$qry_create_mails_detail = "CREATE TABLE `mails_detail` ("
		."`no` int(10) unsigned NOT NULL AUTO_INCREMENT,"
		."`mail_no` int(10) unsigned NOT NULL,"
		."`try_cnt` tinyint(4) NOT NULL,"
		."`smtp_step` tinyint(4) NOT NULL,"
		."`smtp_code` smallint(6) NOT NULL,"
		."`explain` varchar(200) DEFAULT NULL,"
		."`result_code` smallint(6) DEFAULT NULL,"
		."PRIMARY KEY (`no`),"
		."UNIQUE KEY `mail_no` (`mail_no`,`try_cnt`)"
		.") ENGINE=InnoDB DEFAULT CHARSET=euckr;";


		
		if(($dbname == NULL)||($dbname == "")){
			echo "[ERR]Check DB Name[$dbname]\n" ;
			return false;
		}
		else{
			$dbconn = mysql_connect($ip, $id, $pw);
			//echo "[CONNECT TO][$ip][$id][$pw][$dbname]\n";
			$retval = mysql_query($qry_create_db, $dbconn);
			//echo "retval[CREATE]:$retval\n";
			$query  = "use $dbname";
			$retval = mysql_query($query, $dbconn);
			//echo "retval[USE DB]:$retval\n";
			$retval = mysql_query($qry_create_mails, $dbconn);
			//echo "retval[mails]:$retval\n";
			$retval = mysql_query($qry_create_mails_detail, $dbconn);
			//echo "retval[mails_detail]:$retval\n";
			
			mysql_close($dbconn);		
			//echo "[SUCC]Check DB Name[$dbname]\n" ;
			return true;
		}
	}

	//-----------------------------------------------
	// Create DB GService Tables('mails', 'mails_detail')
	//-----------------------------------------------
	function create_gservice_tables($ip, $id, $pw, $dbname)
	{
		$qry_create_db = "CREATE DATABASE `$dbname` /*!40100 DEFAULT CHARACTER SET euckr */;";
		
		$qry_create_mails = "CREATE TABLE `mails` ("
	  ."`no` int(11) unsigned NOT NULL AUTO_INCREMENT,"
	  ."`mailidx` int(10) unsigned NOT NULL,"
	  ."`campaign_no` bigint(20) unsigned NOT NULL,"
	  ."`to_name` varchar(100) DEFAULT NULL,"
	  ."`to_id` varchar(255) NOT NULL DEFAULT '',"
	  ."`to_domain` varchar(255) NOT NULL DEFAULT '',"
	  ."`send_date` datetime NOT NULL,"
	  ."`smtp_step` tinyint(4) NOT NULL DEFAULT '-1',"
	  ."`smtp_code` smallint(6) NOT NULL DEFAULT '0',"
	  ."`try_cnt` tinyint(4) NOT NULL DEFAULT '0',"
	  ."`email` varchar(255) NOT NULL DEFAULT '',"
	  ."`wide_str` text,"
	  ."`basic_info_no` int(10) DEFAULT NULL,"
	  ."`master_user_no` int(10) DEFAULT NULL,"
	  ."PRIMARY KEY (`no`),"
	  ."KEY `campaign_no` (`campaign_no`,`smtp_step`,`try_cnt`),"
	  ."KEY `campaign_no_2` (`campaign_no`,`send_date`),"
	  ."KEY `mailidx` (`mailidx`)"
		.") ENGINE=InnoDB DEFAULT CHARSET=euckr;";


		
		$qry_create_mails_detail = "CREATE TABLE `mails_detail` ("
		."`no` int(10) unsigned NOT NULL AUTO_INCREMENT,"
		."`mail_no` int(10) unsigned NOT NULL,"
		."`try_cnt` tinyint(4) NOT NULL,"
		."`smtp_step` tinyint(4) NOT NULL,"
		."`smtp_code` smallint(6) NOT NULL,"
		."`explain` varchar(200) DEFAULT NULL,"
		."`result_code` smallint(6) DEFAULT NULL,"
		."PRIMARY KEY (`no`),"
		."UNIQUE KEY `mail_no` (`mail_no`,`try_cnt`)"
		.") ENGINE=InnoDB DEFAULT CHARSET=euckr;";


		
		if(($dbname == NULL)||($dbname == "")){
			echo "[ERR]Check DB Name[$dbname]\n" ;
			return false;
		}
		else{
			$dbconn = mysql_connect($ip, $id, $pw);
			//echo "[CONNECT TO][$ip][$id][$pw][$dbname]\n";
			$retval = mysql_query($qry_create_db, $dbconn);
			//echo "retval[CREATE]:$retval\n";
			$query  = "use $dbname";
			$retval = mysql_query($query, $dbconn);
			//echo "retval[USE DB]:$retval\n";
			$retval = mysql_query($qry_create_mails, $dbconn);
			//echo "retval[mails]:$retval\n";
			$retval = mysql_query($qry_create_mails_detail, $dbconn);
			//echo "retval[mails_detail]:$retval\n";
			
			mysql_close($dbconn);		
			//echo "[SUCC]Check DB Name[$dbname]\n" ;
			return true;
		}
	}

	//-----------------------------------------------
	// Create DB (neoems2) AService Tables('mails', 'mails_detail')
	//-----------------------------------------------
	function create_aservice_tables($ip, $id, $pw, $dbname)
	{
		$qry_create_db = "CREATE DATABASE `$dbname` /*!40100 DEFAULT CHARACTER SET utf8 */;";
		
		$qry_create_mails = "CREATE TABLE `mails` ("
	  ."`no` int(11) unsigned NOT NULL AUTO_INCREMENT,"
	  ."`mailidx` int(10) unsigned NOT NULL,"
	  ."`campaign_no` bigint(20) unsigned NOT NULL,"
	  ."`to_name` varchar(100) DEFAULT NULL,"
	  ."`to_id` varchar(255) NOT NULL DEFAULT '',"
	  ."`to_domain` varchar(255) NOT NULL DEFAULT '',"
	  ."`send_date` datetime NOT NULL,"
	  ."`smtp_step` tinyint(4) NOT NULL DEFAULT '-1',"
	  ."`smtp_code` smallint(6) NOT NULL DEFAULT '0',"
	  ."`try_cnt` tinyint(4) NOT NULL DEFAULT '0',"
	  ."`email` varchar(255) NOT NULL DEFAULT '',"
	  ."`wide_str` text,"
	  ."`basic_info_no` int(10) DEFAULT NULL,"
	  ."`master_user_no` int(10) DEFAULT NULL,"
	  ."PRIMARY KEY (`no`),"
	  ."KEY `campaign_no` (`campaign_no`,`smtp_step`,`try_cnt`),"
	  ."KEY `campaign_no_2` (`campaign_no`,`send_date`),"
	  ."KEY `mailidx` (`mailidx`)"
		.") ENGINE=InnoDB DEFAULT CHARSET=utf8;";


		
		$qry_create_mails_detail = "CREATE TABLE `mails_detail` ("
		."`no` int(10) unsigned NOT NULL AUTO_INCREMENT,"
		."`mail_no` int(10) unsigned NOT NULL,"
		."`try_cnt` tinyint(4) NOT NULL,"
		."`smtp_step` tinyint(4) NOT NULL,"
		."`smtp_code` smallint(6) NOT NULL,"
		."`explain` varchar(200) DEFAULT NULL,"
		."`result_code` smallint(6) DEFAULT NULL,"
		."PRIMARY KEY (`no`),"
		."UNIQUE KEY `mail_no` (`mail_no`,`try_cnt`)"
		.") ENGINE=InnoDB DEFAULT CHARSET=utf8;";


		
		if(($dbname == NULL)||($dbname == "")){
			echo "[ERR]Check DB Name[$dbname]\n" ;
			return false;
		}
		else{
			$dbconn = mysql_connect($ip, $id, $pw);
			//echo "[CONNECT TO][$ip][$id][$pw][$dbname]\n";
			$retval = mysql_query($qry_create_db, $dbconn);
			//echo "retval[CREATE]:$retval\n";
			$query  = "use $dbname";
			$retval = mysql_query($query, $dbconn);
			//echo "retval[USE DB]:$retval\n";
			$retval = mysql_query($qry_create_mails, $dbconn);
			//echo "retval[mails]:$retval\n";
			$retval = mysql_query($qry_create_mails_detail, $dbconn);
			//echo "retval[mails_detail]:$retval\n";
			
			mysql_close($dbconn);		
			//echo "[SUCC]Check DB Name[$dbname]\n" ;
			return true;
		}
	}


	//-----------------------------------------------
	// Create DB (neoems2) Build Hosting Tables('mails', 'mails_detail')
	//-----------------------------------------------
	function create_buildhosting_tables($ip, $id, $pw, $dbname)
	{
		$qry_create_db = "CREATE DATABASE `$dbname` /*!40100 DEFAULT CHARACTER SET utf8 */;";
				
		$qry_create_mails = "CREATE TABLE `mails` ("
		."`no` int(11) unsigned NOT NULL AUTO_INCREMENT,"
		."`mailidx` int(10) unsigned NOT NULL,"
		."`campaign_no` bigint(20) unsigned NOT NULL,"
		."`to_name` varchar(100) DEFAULT NULL,"
		."`to_id` varchar(255) NOT NULL DEFAULT '',"
		."`to_domain` varchar(255) NOT NULL DEFAULT '',"
		."`send_date` datetime NOT NULL,"
		."`smtp_step` tinyint(4) NOT NULL DEFAULT '-1',"
		."`smtp_code` smallint(6) NOT NULL DEFAULT '0',"
		."`try_cnt` tinyint(4) NOT NULL DEFAULT '0',"
		."`email` varchar(255) NOT NULL DEFAULT '',"
		."`wide_str` text,"
		."PRIMARY KEY (`no`),"
		."KEY `campaign_no` (`campaign_no`,`smtp_step`,`try_cnt`),"
		."KEY `campaign_no_2` (`campaign_no`,`send_date`),"
		."KEY `mailidx` (`mailidx`)"
		.") ENGINE=InnoDB DEFAULT CHARSET=utf8;";
		
		
		
		$qry_create_mails_detail = "CREATE TABLE `mails_detail` ("
		."`no` int(10) unsigned NOT NULL AUTO_INCREMENT,"
		."`mail_no` int(10) unsigned NOT NULL,"
		."`try_cnt` tinyint(4) NOT NULL,"
		."`smtp_step` tinyint(4) NOT NULL,"
		."`smtp_code` smallint(6) NOT NULL,"
		."`explain` varchar(200) DEFAULT NULL,"
		."`result_code` smallint(6) DEFAULT NULL,"
		."PRIMARY KEY (`no`),"
		."UNIQUE KEY `mail_no` (`mail_no`,`try_cnt`)"
		.") ENGINE=InnoDB DEFAULT CHARSET=utf8;";

		
		if(($dbname == NULL)||($dbname == "")){
			echo "[ERR]Check DB Name[$dbname]\n" ;
			return false;
		}
		else{
			$dbconn = mysql_connect($ip, $id, $pw);
			//echo "[CONNECT TO][$ip][$id][$pw][$dbname]\n";
			$retval = mysql_query($qry_create_db, $dbconn);
			//echo "retval[CREATE]:$retval\n";
			$query  = "use $dbname";
			$retval = mysql_query($query, $dbconn);
			//echo "retval[USE DB]:$retval\n";
			$retval = mysql_query($qry_create_mails, $dbconn);
			//echo "retval[mails]:$retval\n";
			$retval = mysql_query($qry_create_mails_detail, $dbconn);
			//echo "retval[mails_detail]:$retval\n";
			
			mysql_close($dbconn);		
			//echo "[SUCC]Check DB Name[$dbname]\n" ;
			return true;
		}
	}


	define('DB_MAX_COUNT', 2);
	define('DB_NOT_EXIST', 0);
	define('DB_EXIST',     1);

	date_default_timezone_set('Asia/Seoul');
	$time = time();


	//1) DB 연결 정보
	$db_id       = "hiworks";
	$db_password = "fullmoon";
	$main_db_ip  = "127.0.0.1";

	//-------------------
	//DB_Tax_Table
	//-------------------
	//DB 이름을 생성한다.
	$name_header = "neoems_";
	$chk_dbname[0] = $name_header.date("Ym");
	
	for($iCount=1; $iCount < DB_MAX_COUNT; $iCount++){
		$next_month = strtotime("first day of +$iCount month", $time); 
		$chk_dbname[$iCount] = $name_header.date("Ym", $next_month);
	}

	$main_conn   = mysql_connect($main_db_ip, $db_id, $db_password);
	echo "[CONNECT TO][$main_db_ip]\n";

	$dbcheckflag[0]=0;

	for($iCount = 0; $iCount < DB_MAX_COUNT; $iCount++){
		//echo "DBName :[ $chk_dbname[$iCount] ]\n";
		$dbcheckflag[$iCount] = checkdbexist($chk_dbname[$iCount]);
		$checkQuery = "SELECT COUNT(*) AS CC FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA='$chk_dbname[$iCount]' AND TABLE_NAME IN ('mails', 'mails_detail')";
	
		$res = mysql_query($checkQuery, $main_conn);
		//echo "QUERY:$checkQuery \n";

		if(!$res){
			echo "return values is null\n";
			$dbcheckflag[$iCount] = 0;
			continue;
		}else{
			$row_count = mysql_num_rows ($res);
			//echo "\nResource return count:[$row_count]\n";
			if($row_count <= 0){
				echo "Resource Count is null\n";
				$dbcheckflag[$iCount] = DB_NOT_EXIST;
			}
			else {
				$one = mysql_Fetch_Array($res);
				$retCC = $one['CC'];
				if($retCC > 0){
					//echo "retCC is [$retCC] <== Table Exist($iCount)\n";
					$dbcheckflag[$iCount] = DB_EXIST;
				}
				else{
					//echo "retCC is [$retCC]: DB Name '$chk_dbname[$iCount]' is not exist \n";
					$dbcheckflag[$iCount] = DB_NOT_EXIST;
				}
			}
		}
	
		if($res)
			mysql_free_result($res);
	}
	mysql_close($main_conn);		

	
	for($iCount = 0; $iCount < DB_MAX_COUNT; $iCount++){
		echo "$chk_dbname[$iCount] : $dbcheckflag[$iCount] \n";
		if($dbcheckflag[$iCount]==0){
			echo "Create DB:$chk_dbname[$iCount]\n";
			create_tables($main_db_ip, $db_id, $db_password, $chk_dbname[$iCount]);
		}
	}

	//-------------------
	//DB_Tax_Table
	//-------------------
	//DB 이름을 생성한다.
	$name_header = "neoems_tax_";
	$chk_dbname[0] = $name_header.date("Ym");
	
	for($iCount=1; $iCount < DB_MAX_COUNT; $iCount++){
		$next_month = strtotime("first day of +$iCount month", $time); 
		$chk_dbname[$iCount] = $name_header.date("Ym", $next_month);
	}

	$main_conn   = mysql_connect($main_db_ip, $db_id, $db_password);
	echo "[CONNECT TO][$main_db_ip]\n";

	$dbcheckflag[0]=0;

	for($iCount = 0; $iCount < DB_MAX_COUNT; $iCount++){
		//echo "DBName :[ $chk_dbname[$iCount] ]\n";
		$dbcheckflag[$iCount] = checkdbexist($chk_dbname[$iCount]);
		$checkQuery = "SELECT COUNT(*) AS CC FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA='$chk_dbname[$iCount]' AND TABLE_NAME IN ('mails', 'mails_detail')";
	
		$res = mysql_query($checkQuery, $main_conn);
		//echo "QUERY:$checkQuery \n";

		if(!$res){
			echo "return values is null\n";
			$dbcheckflag[$iCount] = 0;
			continue;
		}else{
			$row_count = mysql_num_rows ($res);
			//echo "\nResource return count:[$row_count]\n";
			if($row_count <= 0){
				echo "Resource Count is null\n";
				$dbcheckflag[$iCount] = DB_NOT_EXIST;
			}
			else {
				$one = mysql_Fetch_Array($res);
				$retCC = $one['CC'];
				if($retCC > 0){
					//echo "retCC is [$retCC] <== Table Exist($iCount)\n";
					$dbcheckflag[$iCount] = DB_EXIST;
				}
				else{
					//echo "retCC is [$retCC]: DB Name '$chk_dbname[$iCount]' is not exist \n";
					$dbcheckflag[$iCount] = DB_NOT_EXIST;
				}
			}
		}
	
		if($res)
			mysql_free_result($res);
	}
	mysql_close($main_conn);		

	
	for($iCount = 0; $iCount < DB_MAX_COUNT; $iCount++){
		echo "$chk_dbname[$iCount] : $dbcheckflag[$iCount] \n";
		if($dbcheckflag[$iCount]==0){
			echo "Create DB:$chk_dbname[$iCount]\n";
			create_tax_tables($main_db_ip, $db_id, $db_password, $chk_dbname[$iCount]);
		}
	}

	//-------------------
	//DB_Service_Table
	//-------------------
	//DB 이름을 생성한다.
	$name_header = "neoems_service_";
	$chk_dbname[0] = $name_header.date("Ym");
	
	for($iCount=1; $iCount < DB_MAX_COUNT; $iCount++){
		$next_month = strtotime("first day of +$iCount month", $time); 
		$chk_dbname[$iCount] = $name_header.date("Ym", $next_month);
	}

	$main_conn   = mysql_connect($main_db_ip, $db_id, $db_password);
	echo "[CONNECT TO][$main_db_ip]\n";

	$dbcheckflag[0]=0;

	for($iCount = 0; $iCount < DB_MAX_COUNT; $iCount++){
		//echo "DBName :[ $chk_dbname[$iCount] ]\n";
		$dbcheckflag[$iCount] = checkdbexist($chk_dbname[$iCount]);
		$checkQuery = "SELECT COUNT(*) AS CC FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA='$chk_dbname[$iCount]' AND TABLE_NAME IN ('mails', 'mails_detail')";
	
		$res = mysql_query($checkQuery, $main_conn);
		//echo "QUERY:$checkQuery \n";

		if(!$res){
			echo "return values is null\n";
			$dbcheckflag[$iCount] = 0;
			continue;
		}else{
			$row_count = mysql_num_rows ($res);
			//echo "\nResource return count:[$row_count]\n";
			if($row_count <= 0){
				echo "Resource Count is null\n";
				$dbcheckflag[$iCount] = DB_NOT_EXIST;
			}
			else {
				$one = mysql_Fetch_Array($res);
				$retCC = $one['CC'];
				if($retCC > 0){
					//echo "retCC is [$retCC] <== Table Exist($iCount)\n";
					$dbcheckflag[$iCount] = DB_EXIST;
				}
				else{
					//echo "retCC is [$retCC]: DB Name '$chk_dbname[$iCount]' is not exist \n";
					$dbcheckflag[$iCount] = DB_NOT_EXIST;
				}
			}
		}
	
		if($res)
			mysql_free_result($res);
	}
	mysql_close($main_conn);		

	
	for($iCount = 0; $iCount < DB_MAX_COUNT; $iCount++){
		echo "$chk_dbname[$iCount] : $dbcheckflag[$iCount] \n";
		if($dbcheckflag[$iCount]==0){
			echo "Create DB:$chk_dbname[$iCount]\n";
			create_service_tables($main_db_ip, $db_id, $db_password, $chk_dbname[$iCount]);
		}
	}

	//-------------------
	//DB_GService_Table
	//-------------------
	//DB 이름을 생성한다.
	$name_header = "neoems_gservice_";
	$chk_dbname[0] = $name_header.date("Ym");
	
	for($iCount=1; $iCount < DB_MAX_COUNT; $iCount++){
		$next_month = strtotime("first day of +$iCount month", $time); 
		$chk_dbname[$iCount] = $name_header.date("Ym", $next_month);
	}

	$main_conn   = mysql_connect($main_db_ip, $db_id, $db_password);
	echo "[CONNECT TO][$main_db_ip]\n";

	$dbcheckflag[0]=0;

	for($iCount = 0; $iCount < DB_MAX_COUNT; $iCount++){
		//echo "DBName :[ $chk_dbname[$iCount] ]\n";
		$dbcheckflag[$iCount] = checkdbexist($chk_dbname[$iCount]);
		$checkQuery = "SELECT COUNT(*) AS CC FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA='$chk_dbname[$iCount]' AND TABLE_NAME IN ('mails', 'mails_detail')";
	
		$res = mysql_query($checkQuery, $main_conn);
		//echo "QUERY:$checkQuery \n";

		if(!$res){
			echo "return values is null\n";
			$dbcheckflag[$iCount] = 0;
			continue;
		}else{
			$row_count = mysql_num_rows ($res);
			//echo "\nResource return count:[$row_count]\n";
			if($row_count <= 0){
				echo "Resource Count is null\n";
				$dbcheckflag[$iCount] = DB_NOT_EXIST;
			}
			else {
				$one = mysql_Fetch_Array($res);
				$retCC = $one['CC'];
				if($retCC > 0){
					//echo "retCC is [$retCC] <== Table Exist($iCount)\n";
					$dbcheckflag[$iCount] = DB_EXIST;
				}
				else{
					//echo "retCC is [$retCC]: DB Name '$chk_dbname[$iCount]' is not exist \n";
					$dbcheckflag[$iCount] = DB_NOT_EXIST;
				}
			}
		}
	
		if($res)
			mysql_free_result($res);
	}
	mysql_close($main_conn);		

	
	for($iCount = 0; $iCount < DB_MAX_COUNT; $iCount++){
		echo "$chk_dbname[$iCount] : $dbcheckflag[$iCount] \n";
		if($dbcheckflag[$iCount]==0){
			echo "Create DB:$chk_dbname[$iCount]\n";
			create_gservice_tables($main_db_ip, $db_id, $db_password, $chk_dbname[$iCount]);
		}
	}

	//-------------------
	//DB_AService_Table
	//-------------------
	//DB 이름을 생성한다.
	$name_header = "neoems2_aservice_";
	$chk_dbname[0] = $name_header.date("Ym");
	
	for($iCount=1; $iCount < DB_MAX_COUNT; $iCount++){
		$next_month = strtotime("first day of +$iCount month", $time); 
		$chk_dbname[$iCount] = $name_header.date("Ym", $next_month);
	}

	$main_conn   = mysql_connect($main_db_ip, $db_id, $db_password);
	echo "[CONNECT TO][$main_db_ip]\n";

	$dbcheckflag[0]=0;

	for($iCount = 0; $iCount < DB_MAX_COUNT; $iCount++){
		//echo "DBName :[ $chk_dbname[$iCount] ]\n";
		$dbcheckflag[$iCount] = checkdbexist($chk_dbname[$iCount]);
		$checkQuery = "SELECT COUNT(*) AS CC FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA='$chk_dbname[$iCount]' AND TABLE_NAME IN ('mails', 'mails_detail')";
	
		$res = mysql_query($checkQuery, $main_conn);
		//echo "QUERY:$checkQuery \n";

		if(!$res){
			echo "return values is null\n";
			$dbcheckflag[$iCount] = 0;
			continue;
		}else{
			$row_count = mysql_num_rows ($res);
			//echo "\nResource return count:[$row_count]\n";
			if($row_count <= 0){
				echo "Resource Count is null\n";
				$dbcheckflag[$iCount] = DB_NOT_EXIST;
			}
			else {
				$one = mysql_Fetch_Array($res);
				$retCC = $one['CC'];
				if($retCC > 0){
					//echo "retCC is [$retCC] <== Table Exist($iCount)\n";
					$dbcheckflag[$iCount] = DB_EXIST;
				}
				else{
					//echo "retCC is [$retCC]: DB Name '$chk_dbname[$iCount]' is not exist \n";
					$dbcheckflag[$iCount] = DB_NOT_EXIST;
				}
			}
		}
	
		if($res)
			mysql_free_result($res);
	}
	mysql_close($main_conn);		

	
	for($iCount = 0; $iCount < DB_MAX_COUNT; $iCount++){
		echo "$chk_dbname[$iCount] : $dbcheckflag[$iCount] \n";
		if($dbcheckflag[$iCount]==0){
			echo "Create DB:$chk_dbname[$iCount]\n";
			create_aservice_tables($main_db_ip, $db_id, $db_password, $chk_dbname[$iCount]);
		}
	}


	//-------------------
	//DB_Build_Hosing_Table
	//-------------------
	//DB 이름을 생성한다.
	$name_header = "neoems2_";
	$chk_dbname[0] = $name_header.date("Ym");
	
	for($iCount=1; $iCount < DB_MAX_COUNT; $iCount++){
		$next_month = strtotime("first day of +$iCount month", $time); 
		$chk_dbname[$iCount] = $name_header.date("Ym", $next_month);
	}

	$main_conn   = mysql_connect($main_db_ip, $db_id, $db_password);
	echo "[CONNECT TO][$main_db_ip]\n";

	$dbcheckflag[0]=0;

	for($iCount = 0; $iCount < DB_MAX_COUNT; $iCount++){
		//echo "DBName :[ $chk_dbname[$iCount] ]\n";
		$dbcheckflag[$iCount] = checkdbexist($chk_dbname[$iCount]);
		$checkQuery = "SELECT COUNT(*) AS CC FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA='$chk_dbname[$iCount]' AND TABLE_NAME IN ('mails', 'mails_detail')";
	
		$res = mysql_query($checkQuery, $main_conn);
		//echo "QUERY:$checkQuery \n";

		if(!$res){
			echo "return values is null\n";
			$dbcheckflag[$iCount] = 0;
			continue;
		}else{
			$row_count = mysql_num_rows ($res);
			//echo "\nResource return count:[$row_count]\n";
			if($row_count <= 0){
				echo "Resource Count is null\n";
				$dbcheckflag[$iCount] = DB_NOT_EXIST;
			}
			else {
				$one = mysql_Fetch_Array($res);
				$retCC = $one['CC'];
				if($retCC > 0){
					//echo "retCC is [$retCC] <== Table Exist($iCount)\n";
					$dbcheckflag[$iCount] = DB_EXIST;
				}
				else{
					//echo "retCC is [$retCC]: DB Name '$chk_dbname[$iCount]' is not exist \n";
					$dbcheckflag[$iCount] = DB_NOT_EXIST;
				}
			}
		}
	
		if($res)
			mysql_free_result($res);
	}
	mysql_close($main_conn);		

	
	for($iCount = 0; $iCount < DB_MAX_COUNT; $iCount++){
		echo "$chk_dbname[$iCount] : $dbcheckflag[$iCount] \n";
		if($dbcheckflag[$iCount]==0){
			echo "Create DB:$chk_dbname[$iCount]\n";
			create_buildhosting_tables($main_db_ip, $db_id, $db_password, $chk_dbname[$iCount]);
		}
	}
	
	
	return;
?>

