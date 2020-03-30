<?
	$fp = fsockopen("121.78.234.20", 5096, $errno, $errstr, 30);

	if(!$fp)
	{
		echo "$errstr ($errno)\n";
		exit;
	}

	while( true )
	{

	$chat = "Hello world!\0";
	$ulSize = 8 + strlen($chat);
	$ulType = 1000;		// MSG_CHAT

	$a = ($ulSize & 0xff000000) >> 24;	
	$b = ($ulSize & 0x00ff0000) >> 16;	
	$c = ($ulSize & 0x0000ff00) >> 8;	
	$d = $ulSize & 0x000000ff;

	fputs($fp, chr($a).chr($b).chr($c).chr($d), 4);
	
	$a = ($ulType & 0xff000000) >> 24;	
	$b = ($ulType & 0x00ff0000) >> 16;	
	$c = ($ulType & 0x0000ff00) >> 8;	
	$d = $ulType & 0x000000ff;

	fputs($fp, chr($a).chr($b).chr($c).chr($d), 4);
	fputs($fp, $chat, strlen($chat));
	fflush($fp);

	$a = fread($fp, 1);
	$b = fread($fp, 1);
	$c = fread($fp, 1);
	$d = fread($fp, 1);

	$ulSize = ord($a) << 24 | ord($b) << 16 | ord($c) << 8 | ord($d);
	echo "ulSize : " . $ulSize . "\n";

	$a = fread($fp, 1);
	$b = fread($fp, 1);
	$c = fread($fp, 1);
	$d = fread($fp, 1);

	$ulType = ord($a) << 24 | ord($b) << 16 | ord($c) << 8 | ord($d);
	echo "ulType : " . $ulType . "\n";

	$chat = fread($fp, $ulSize - 8);
	echo "Server said : " . $chat . "\n";
	}	
?>
