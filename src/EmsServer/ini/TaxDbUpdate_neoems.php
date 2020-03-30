#!/usr/local/php -f
<?php
	function ProcessCheck()
	{
		$m_ExeStr = "ps -ef | grep TaxDbUpdate_neoems.php 2>&1";
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
	
	$mShowLog = 0;
	
	
	if(ProcessCheck() == 1)
	{
		if($mShowLog){
			echo "Already Exist\n";
		}
		exit;
	}	
	
	//이번달과 지난달을 이용하여 DB 이름을 생성한다.
	$name_header = "neoems_tax_";   //접두어는 변경될 것임.
	date_default_timezone_set('Asia/Seoul');
	$time = time();
	$prev_month = strtotime("first day of -1 month",  $time); 
	
	define('MAX_DB_COUNT', 2);
	
	$dbname[0] = $name_header.date("Ym", $prev_month);
	$dbname[1] = $name_header.date("Ym"); 
	
	//1) DB 연결
	$db_id       = "root";
	$db_password = "rkqldk12#$";
	$main_db_ip  = "127.0.0.1";

	$main_conn   = mysql_connect($main_db_ip,$db_id,$db_password);
	if($mShowLog)
		echo "\n[CONNECT TO][$main_db_ip][$db_id][$db_password]\n";

	$mConnectedSubIp = "";
	$sub_conn        = NULL;

	//2)반복구간: 변경사항이 있는 필드 세금 계산서 디비 업데이트 처리
	for($idbcount = 0; $idbcount < MAX_DB_COUNT; $idbcount++){
		//'neoems' DB에 접속하여 세금계산서 항목의 변경된 항목을 추출하여 리스트를 가져온다.
		// 리스트는 순차방식으로...

		while(true){ //해당 디비 처리 필드 없을 때까지 진행하기 위한 반복 구간
			//$dbname[$idbcount]
			$main_query = sprintf("SELECT CM.campaign_no, CM.mailidx, CM.db_ip, CM.db_partitioning, CM.document_id, CM.smtp_step, CM.smtp_code, CE.explain, CE.result_code FROM $dbname[$idbcount].mails CM LEFT JOIN $dbname[$idbcount].mails_detail CE ON CM.no=CE.mail_no AND CM.try_cnt = CE.try_cnt WHERE ApplyTaxdb='N' and CM.smtp_step>-1 order by db_ip limit 1000");	
			if($mShowLog)
				echo "\n[Main_Query:$main_query]\n";
			$res = mysql_query($main_query, $main_conn);
			if(!$res){
				break;
			}else{
				$row_count = mysql_num_rows ($res);
				if($mShowLog)
					echo "\nResource return count:$row_count\n";
				if($row_count <= 0){
					break;
				}
			}
			
			//필드가 존재한다면, 가져와서 업데이트를 처리한다.
			
			while($one = mysql_Fetch_Array($res))
			{
				$m_campaign_no  = $one['campaign_no'];
				$m_mailidx      = $one['mailidx'];
				$m_db_ip        = $one['db_ip'];
				$m_db_partition = $one['db_partitioning'];
				$m_document_id  = $one['document_id'];
				$m_smtp_step    = $one['smtp_step'];
				$m_smtp_code    = $one['smtp_code'];
				$m_explain      = $one['explain'];
				$m_result_code  = $one['result_code'];
				
				if($mShowLog)
					echo "$m_campaign_no : $m_mailidx : $m_db_ip :$m_db_partition : $m_document_id : $m_smtp_step : $m_smtp_code : $m_explain : $m_result_code \n";

				if($m_db_partition == NULL || $m_document_id == NULL || $m_db_ip == NULL)
				{
					$main_query = sprintf("UPDATE %s.mails SET ApplyTaxdb='Y' WHERE ApplyTaxdb='N' and campaign_no=%d and mailidx=%d", $dbname[$idbcount], $m_campaign_no, $m_mailidx);
					if($mShowLog)
						echo "main_query : $main_query";
					$main_return = mysql_query($main_query, $main_conn);	
								
					continue;
				}
			
				if($mConnectedSubIp != $m_db_ip)
				{
					if($mConnectedSubIp != "")
					{
						if($sub_conn != NULL)
							mysql_close($sub_conn);					
						$sub_conn = NULL;
						$mConnectedSubIp = "";
					}
					
					if($sub_conn == NULL)
					{
						$sub_conn = mysql_connect($m_db_ip, $db_id, $db_password);
						
						if($sub_conn == NULL)
						{
							$mConnectedSubIp = "";
							echo "sub db connect err-".$m_db_ip."\n";
							continue;
							//exit;
						}
						
						if($mShowLog)
							echo "Sub Connect ".$m_db_ip."\n";				
									
						$mConnectedSubIp = $m_db_ip;
					}
				}
				
				$sub_query = sprintf("UPDATE bill_%s.document_sign SET SmtpStep=%d,SmtpCode=%d,`Explain`='%s',ResultCode=%d WHERE document_id='%s'"
				                        , $m_db_partition, $m_smtp_step, $m_smtp_code, $m_explain, $m_result_code, $m_document_id);
				if($mShowLog){
					echo "[sub_query : $sub_query]";
				}
				
				$sub_return = mysql_query($sub_query, $sub_conn);	
				
				if($sub_return){
					if($mShowLog){
						echo "\n".$sub_query."[SUCCESS]"."\n";
					}
				}
				else{
					echo "\n".$sub_query."[FAIL]"."\n";
				}
				
				if($sub_return)
				{
					$main_query = sprintf("UPDATE $dbname[$idbcount].mails  SET ApplyTaxdb='Y' WHERE ApplyTaxdb='N' AND campaign_no='%s' AND  mailidx=%s", $m_campaign_no, $m_mailidx);
					$main_return = mysql_query($main_query, $main_conn);	
					
					if($main_return){
						if($mShowLog){
							echo "\n".$main_query."[SUCCESS]"."\n";
						}
					}
					else{
						echo "\n".$main_query."[FAIL]"."\n";
					}
				}
			}
		}
	}
	
	if($sub_conn != NULL)
		mysql_close($sub_conn);
		
	if($res)
		mysql_free_result($res);

	if($main_conn != NULL)
		mysql_close($main_conn);		
	
	if($mShowLog){
		echo "\n"."-END-"."\n";
	}
	
?>
