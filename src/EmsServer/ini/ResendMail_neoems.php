#!/usr/local/php -f
<?php
	function ProcessCheck()
	{
		$m_ExeStr = "ps -ef | grep ResendMail_neoems.php | grep -v grep| grep -v cat| grep -v tail  2>&1";
		$m_ExeResult;
		$m_phpCnt = 0;
		
		exec($m_ExeStr, $m_ExeResult);

		foreach($m_ExeResult as $value)
		{
			if(stripos($value, "ResendMail_neoems.php") > 0)
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
		if($mShowLog){
			echo "Already Exist\n";
		}
		exit;
	}	

//neoems에서 실패한 메일 리스트를 기간으로 수집(type[M,I]를 분리하여 처리. 최대 한 번에 100개씩)
//M: 대량 메일 전송이므로 해당 수신 메일의 상태가 발송으로 넘어간 상태에서 36시간이 지났는지를 확인.
//I: 개인 메일 전송이므로 발송 상태가 -1이 아닌 상태의 값 중에 24시간이 지났는지를 기준으로 재발송 처리

//시나리오
//1. neoems에서 종료되지 않은 리스트(0 또는 1)인 것들을 가져온다.(type 'I')
// - (발송 시간 기준으로 24시간 이상 발송 처리(성공/실패)되지 않은 건에 대해서 재발송 처리한다)

//2. neoems에서 종료되지 않은 리스트(0 또는 1)인 것들을 가져온다.(type 'M')
// - (발송 시간 기준으로 36시간 이상 발송 처리(성공/실패)되지 않은 건에 대해서 재발송 처리한다)

	date_default_timezone_set('Asia/Seoul');
	
	$time = time();
	$prevDateTime = strtotime("-24 hour",  $time);
	$checkDateTime = date("Y-m-d H:i:s", $prevDateTime);
	//1) DB 연결
	$db_id        = "root";
	$db_password  = "rkqldk12#$";
	$main_db_ip   = "127.0.0.1";
	$main_db_name = "neoems";

	$icount           = 0;
	$cp_arr_count_I   = 0;
	$cp_arr_count_M   = 0;
	$campaign_arr_I[] = null;
	$campaign_arr_M[] = null;

	$main_conn    = mysql_connect($main_db_ip,$db_id,$db_password);
	if($main_conn == false){
		if($mShowLog){
			echo "[CONNECT TO][DB Main Connection Failed][$main_db_ip][$db_id][$db_password]\n";
		}
	}
	else{
		$sub_conn    = mysql_connect($main_db_ip,$db_id,$db_password);
		if($sub_conn == false){
			echo "[CONNECT TO][DB Sub Connection Failed][$main_db_ip][$db_id][$db_password]\n";
		}
	}
	
	if($mShowLog)
		echo "\n[CONNECT TO][$main_db_ip][$db_id][$db_password]\n";


	//-------------------------------------------------------------
	// Mode 'I'와 Mode 'M' 으로 분리하여 미발송된 메일 처리.
	//-------------------------------------------------------------
	//2) Mode 'I' 미발송 리스크 검색
	// neoems 접속하여 Step이 1이고 type `I`인 항목을 찾아 발송상태를 확인하고 재발송 한다.
	//-------------------------------------------------------------
	
	$main_query = sprintf("SELECT no, send_date, step, db_name, tr_type FROM $main_db_name.campaign WHERE step=1 AND tr_type='I' AND send_date < '$checkDateTime' LIMIT 100");
	
	if($mShowLog)
		echo "\n[Main_Query:$main_query]\n";

	$res = mysql_query($main_query, $main_conn);

	if(!$res){ //쿼리가 실패라면 커넥션 끊고 종료
		exit;
	}else{
		$row_count = mysql_num_rows ($res);
		if($mShowLog)
			echo "\nResource return count:$row_count\n";
			
		if($row_count > 0){
			//체크할 리스트를 생성하고 만약 정해진 시간보다 오래된 메시지가 전송되지 않은 상태인 경우 재발송을 시도한다.
			
			while($one = mysql_Fetch_Array($res))
			{
				$m_campaign_no = $one['no'];
				$m_send_date   = $one['send_date'];
				$m_step        = $one['step'];
				$m_db_name     = $one['db_name'];
				$m_tr_type     = $one['tr_type'];
				$m_checkflag   = "false";
				
				if($mShowLog)
					echo "$m_campaign_no : $m_send_date : $m_step : $m_db_name : $m_tr_type \n";

				if($m_send_date == NULL || $m_db_name == NULL )
					continue;
			
				// 해당 정보 저장
				$campaign_arr_I[$icount] = array($m_campaign_no, $m_send_date, $m_db_name, $m_checkflag);
				$icount++;
			}
		}
		if($res)
			mysql_free_result($res);
		
		if($row_count > 0){
			
			$cp_arr_count_I = count($campaign_arr_I);
	
			if($mShowLog){
				echo "Campaing[I] Count: $cp_arr_count_I \n";
			}
			
			for($i = 0; $i < $cp_arr_count_I; $i++){
				$cp_arr  = $campaign_arr_I[$i];
				$cp_no   = $cp_arr[0];
				$db_name = $cp_arr[2];
	
				$sub_query = "UPDATE $db_name.mails SET smtp_step=-1 WHERE campaign_no=$cp_no AND smtp_step>-1 AND smtp_step != 7 AND try_cnt>-1 AND try_cnt < 3 ";
	
				if($mShowLog)
					echo "[sub_query][$sub_query]\n";
	
				$sub_return = mysql_query($sub_query, $sub_conn);
				if($mShowLog)
					echo "[sub_return][$sub_return]\n";
				
				$sub_query = "UPDATE $main_db_name.campaign SET step=-1 WHERE no=$cp_no AND step != 2 ";
	
				if($mShowLog)
					echo "[sub_query][$sub_query]\n";
	
				$sub_return = mysql_query($sub_query, $sub_conn);
				if($mShowLog)
					echo "[sub_return][$sub_return]\n";
			}
		}
		else{
		}
	}
	

	//-------------------------------------------------------------
	//3) Mode 'M' 미발송 리스크 검색
	// neoems 접속하여 step이 1이고 type `M`인 항목을 찾아 발송상태를 확인하고 재발송 한다.
	//-------------------------------------------------------------
	$time = time();
	$prevDateTime  = strtotime("-36 hour",  $time); 
	$checkDateTime = date("Y-m-d H:i:s", $prevDateTime);
	
	$main_query = sprintf("SELECT no, send_date, step, db_name, tr_type FROM $main_db_name.campaign WHERE step=1 AND tr_type='M' AND send_date < '$checkDateTime' LIMIT 10");
	
	if($mShowLog)
		echo "\n[Main_Query:$main_query]\n";

	$res = mysql_query($main_query, $main_conn);

	if(!$res){ //쿼리가 실패라면 커넥션 끊고 종료
		exit;
	}else{
		$row_count = mysql_num_rows ($res);
		if($mShowLog)
			echo "\nResource return count:$row_count\n";
		if($row_count > 0){
			//체크할 리스트를 생성하고 만약 정해진 시간보다 오래된 메시지가 전송되지 않은 상태인 경우 재발송을 시도한다.
			$icount = 0;
			while($one = mysql_Fetch_Array($res))
			{
				$m_campaign_no = $one['no'];
				$m_send_date   = $one['send_date'];
				$m_step        = $one['step'];
				$m_db_name     = $one['db_name'];
				$m_tr_type     = $one['tr_type'];
				$m_checkflag   = "false";
				
				if($mShowLog)
					echo "$m_campaign_no : $m_send_date : $m_step : $m_db_name : $m_tr_type \n";

				if($m_send_date == NULL || $m_db_name == NULL )
					continue;
			
				// 해당 정보 저장
				$campaign_arr_M[$icount] = array($m_campaign_no, $m_send_date, $m_db_name, $m_checkflag);
				$icount++;

			}			
		}
		if($res)
			mysql_free_result($res);
		
		if($row_count > 0){
			
			$cp_arr_count_M = count($campaign_arr_M);

			if($mShowLog)
				echo "Campaing[M] Count: $cp_arr_count_M \n";
			
			for($i = 0; $i < $cp_arr_count_M ; $i++){
				$cp_arr  = $campaign_arr_M[$i];
				$cp_no   = $cp_arr[0];
				$db_name = $cp_arr[2];
	
				$sub_query = "UPDATE $db_name.mails SET smtp_step=-1 WHERE campaign_no=$cp_no AND smtp_step>-1 AND smtp_step != 7 AND try_cnt>-1 AND try_cnt < 3 ";
	
				if($mShowLog)
					echo "[sub_query][$sub_query]\n";
	
				$sub_return = mysql_query($sub_query, $sub_conn);
				if($mShowLog)
					echo "[sub_return][$sub_return]\n";
				
				$sub_query = "UPDATE $main_db_name.campaign SET step=-1 WHERE no=$cp_no AND step != 2 ";
	
				if($mShowLog)
					echo "[sub_query][$sub_query]\n";
	
				$sub_return = mysql_query($sub_query, $sub_conn);
				if($mShowLog)
					echo "[sub_return][$sub_return]\n";
			}
		}
		else{
		}
	}
	// 업데이트 된 경우에 send_date를 원복하기 위해 상태 체크
	{
		$checkout = true;
		do{
			$checkout = true;
			//-------------
			//tr_type 'I'
			//-------------
			if($cp_arr_count_I > 0){
				for($i = 0; $i < $cp_arr_count_I; $i++){
					$cp_arr   = $campaign_arr_I[$i];
					
					if($cp_arr[3]=="false"){
						//send_date 초기 날짜로 업데이트: 프로세스가 처리 중인지 확인
						$main_query = "SELECT step FROM $main_db_name.campaign WHERE no=$cp_arr[0] ";
		
						if($mShowLog)
							echo "[main_query][$main_query]\n";
							
						$res = mysql_query($main_query, $main_conn);
					
						if(!$res){ //쿼리가 실패라면 커넥션 끊고 종료
							exit;
						}else{
							$row_count = mysql_num_rows ($res);
								
							if($row_count > 0){
								//체크할 리스트를 생성하고 만약 정해진 시간보다 오래된 메시지가 전송되지 않은 상태인 경우 재발송을 시도한다.
								
								$one = mysql_Fetch_Array($res);
								$m_step = $one['step'];
								
								if($m_step != -1){
									$sub_query = "UPDATE $main_db_name.campaign SET send_date = '$cp_arr[1]' WHERE no = $cp_arr[0] ";
						
									if($mShowLog)
										echo "[sub_query][$sub_query]\n";
						
									$sub_return = mysql_query($sub_query, $sub_conn);

									if($sub_return != false){
										$campaign_arr_I[$i][3]="true";
									}
		
									if($mShowLog)
										echo "[sub_return][$sub_return][$cp_arr[3]]\n";							
								}
								else{
									$checkout = false;
								}
											
								if($res)
									mysql_free_result($res);
							}
						}
					}
					else{
						continue;
					}

					sleep(1);
				}
				if($checkout == true)
				  $cp_arr_count_I = 0;
			}
			
			$checkout = true;
			//-------------
			//tr_type 'M'
			//-------------
			if($cp_arr_count_M > 0){
				for($i = 0; $i < $cp_arr_count_M; $i++){
					$cp_arr   = $campaign_arr_M[$i];
					
					if($cp_arr[3]=="false"){
						//send_date 초기 날짜로 업데이트: 프로세스가 처리 중인지 확인
						$main_query = "SELECT step FROM $main_db_name.campaign WHERE no=$cp_arr[0] ";
		
						if($mShowLog)
							echo "[main_query][$main_query]\n";
							
						$res = mysql_query($main_query, $main_conn);
					
						if(!$res){ //쿼리가 실패라면 커넥션 끊고 종료
							exit;
						}else{
							$row_count = mysql_num_rows ($res);
								
							if($row_count > 0){
								//체크할 리스트를 생성하고 만약 정해진 시간보다 오래된 메시지가 전송되지 않은 상태인 경우 재발송을 시도한다.
								
								$one = mysql_Fetch_Array($res);
								$m_step = $one['step'];
								
								if($m_step != -1){
									$sub_query = "UPDATE $main_db_name.campaign SET send_date = '$cp_arr[1]' WHERE no = $cp_arr[0] ";
						
									if($mShowLog)
										echo "[sub_query][$sub_query]\n";
						
									$sub_return = mysql_query($sub_query, $sub_conn);

									if($sub_return != false){
										$campaign_arr_m[$i][3]="true";
									}
		
									if($mShowLog)
										echo "[sub_return][$sub_return][$cp_arr[3]]\n";							
								}
								else{
									$checkout = false;
								}

								if($res)
									mysql_free_result($res);
							}
						}
					}
					else{
						continue;
					}
		
					sleep(1);
				}
				
				if($checkout == true){
					$cp_arr_count_M = 0;
				}
			}
		}while($cp_arr_count_I != 0 || $cp_arr_count_M != 0);
	}

	if($sub_conn != NULL)
		mysql_close($sub_conn);
		
	if($main_conn != NULL)
		mysql_close($main_conn);		
	
	if($mShowLog){
		echo "\n"."-END-"."\n";
	}
	
?>
