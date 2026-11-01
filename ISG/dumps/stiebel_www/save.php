<?php
// Session starten
session_start();
$time_a= microtime();
// Kodierung
header("Content-type: application/json");
// Standartvariablen
include('./stablevalues.php');
// Logout + Login + Session abgleich
include('./auth.php');

class sc /* SaveChanges */ extends sf /* StandartFunktionen */ {

  private $warning = false; // \
  private $result=array();  //  }-> Output
  private $errors=array();  // /
  private $data;            // bearbeitungsdaten
  private $pmg = "-g";      // Php_Mapper Get
  private $pms = "-s";      // Php_Mapper Set
  private $oktext = false;
  private $storageResults = array(); // array for storing results
  
  // code
  function set_data($val,$vals){
    $this->data = &$val;
	// Uebler Hack: Spezialfaelle je nach Namen des ersten Input-Objektes in der Form
    if(in_array($this->data[0]->name,array('netzwerk_dhcp','netzwerk_ip_0')))sc::SpeichernNetzwerk($vals);
    elseif($this->data[0]->name=='sicherheit') sc::SpeichernSicherheit($vals);
    elseif($this->data[0]->name=='isganlagegeraetebild') sc::SpeichernGeraetebild($vals);
    elseif($this->data[0]->name=='valspracheeinstellung') sc::SpeichernSprache($vals);
	elseif($this->data[0]->name=='vallandeinstellunglive') sc::SpeichernLandUndSprache($vals);
    else sc::SpeichernStandardForm();
  }
  
  function SpeichernStandardForm() {
  // IDs fuer den phpmapper zur ueberpruefung holen
  for($i=0;$i<count($this->data);$i++){
    $this->data[$i]->name = str_replace("val","",$this->data[$i]->name);
    $this->data[$i]->value = str_replace(",",".",$this->data[$i]->value);
//    $ids[] = $this->data[$i]->name;
  }
  //print_r($this->data);
  /*
  include_once('./obj/php_mapper.php');
  $phpm = new isg_php_mapper;
  
  $oh = $phpm->get_db_obj_ids($ids);
  print_r($oh);
  
  $this->result = array('success' => true, 'message' => $ids);
  $this::output();
  exit;
  */
  
  // unbenutzter historischer Code
  /*
    if($this->data[0]->name=='wtag'){ // fuer schaltprogramme
    $i=0;
    while($this->data[$i]->name=='wtag'){
      $wday[$i]=$this->data[$i]->value;
      $i++;
    }
    for($j=0;$j<((count($this->data)-$i)/4);$j++){
      $web_val[0] = $this->data[$i+($j*4)+0]->value*4;
      $web_val[1] = $this->data[$i+($j*4)+1]->value/15;
      $hour_start = ($web_val[0]+$web_val[1])*256;
      $web_val[2] = $this->data[$i+($j*4)+2]->value*4;
      $web_val[3] = $this->data[$i+($j*4)+3]->value/15;
      $hour_stop = ($web_val[2]+$web_val[3]);
      for($k=0;$k<$i;$k++){
        $web_info[($j*$i)+$k]->name = $this->data[$i+($j*4)]->name+($wday[$k]*((count($this->data)-$i)/4));
        $web_info[($j*$i)+$k]->value = $hour_start+$hour_stop;
      }
    }
    $this->data = &$web_info;
  }else if($this->data->betrieb){
  }else */
  
  //Code für Programme mit Slider
  if( strpos( $this->data[0]->value, ";" ) ){
    for($i=0;$i<count($this->data);$i++){
      $so = explode(";",$this->data[$i]->value);
      $this->data[$i]->value = $so[1]+($so[0]*256);
    }
  }
  // Ferienprogramm LWZ
  else if( substr( $this->data[0]->name, 0, 8 ) == 'feriprog' && substr( $this->data[5]->name, 0, 8 ) == 'feriprog' )
  { 
    $this->data[0]->name = substr( $this->data[0]->name, 8 );
    $this->data[5]->name = substr( $this->data[5]->name, 8 );
    #$this->data[2]->value = substr($this->data[2]->value,-2);
    #$this->data[7]->value = substr($this->data[7]->value,-2);
    $angabenfehler = false;
    
	if (!checkdate($this->data[1]->value, $this->data[0]->value, 2000 + $this->data[2]->value))
		$this->result = array('success' => false, 'message' => 'Ung&uuml;ltiger Anfangszeitpunkt!');
	else
	{
		if (!checkdate($this->data[6]->value, $this->data[5]->value, 2000 + $this->data[7]->value))
			$this->result = array('success' => false, 'message' => 'Ung&uuml;ltiger Endzeitpunkt!');
		else
		{
			$start_timestamp = mktime($this->data[3]->value, $this->data[4]->value, 0, $this->data[1]->value, $this->data[0]->value, $this->data[2]->value);
			$end_timestamp = mktime($this->data[8]->value, $this->data[9]->value, 0, $this->data[6]->value, $this->data[5]->value, $this->data[7]->value);
			if ($end_timestamp <= $start_timestamp)
				$this->result = array('success' => false, 'message' => 'Endzeitpunkt liegt vor Anfangszeitpunkt!');	
			else
			{
				$this->data[3]->value = ($this->data[3]->value*4)+($this->data[4]->value/15);
				$this->data[8]->value = ($this->data[8]->value*4)+($this->data[9]->value/15);
				unset($this->data[9]);
				unset($this->data[4]);
				$this->data = array_merge($this->data);	// Elemente werden neu durchnummeriert
			}
		}
	}
  }
  else if($this->data[0]->name == $this->data[1]->name)
  { // Partyprogramm hat 0,1,2,3 gleichen Wert.
	// Legionellenprogramm hat nur 0,1 den gleichen Wert: 425
    if (($this->data[0]->name == $this->data[2]->name && $this->data[0]->name == $this->data[3]->name) ||
        ($this->data[0]->name == "425"))
    {
      $kleiner_null = false;
      $std_größer_24 = false;
      $min_gültigkeit = array(0,15,30,45);
      $min_außerhalb_der_gültigkeit = false;
      $Uhrzeit_größer_24Uhr = false;
      
      for($i=0;$i<4;$i++)
      {
        if( $this->data[$i]->value < 0 )
        {
          $kleiner_null = true;
          break;
        }
        if( !($i%2) )
        { // stunden
          if( $this->data[$i]->value > 24 )
          {
            $std_größer_24 = true;
            break;
          }
          if( $this->data[$i]->value == 24 && $this->data[($i+1)]->value > 0 )
          {
            $Uhrzeit_größer_24Uhr = true;
            break;
          }
        }
        else
        { // minuten
          if(!in_array( $this->data[$i]->value, $min_gültigkeit) ){
            $min_außerhalb_der_gültigkeit = true;
            break;
          }
        }
      }
      
      if( $kleiner_null || $std_größer_24 || $min_außerhalb_der_gültigkeit || $Uhrzeit_größer_24Uhr )
      {
        $this->result = array('success' => false, 'message' => 'Falsche Angaben!');
        
      }
      else
      {
        $this->data[0]->value*=4;
        $this->data[1]->value/=15;
        $hour_start = ($this->data[0]->value+$this->data[1]->value);
        $this->data[2]->value*=4;
        $this->data[3]->value/=15;
        $hour_stop = ($this->data[2]->value+$this->data[3]->value);
        unset($this->data[1]);
        unset($this->data[2]);
        unset($this->data[3]);
        $this->data[0]->value = $hour_start+($hour_stop*256);
      }
    }
  }
  else if (substr( $this->data[0]->name, 0, 9 ) == 'datecheck')
  {
	$this->data[0]->name = substr($this->data[0]->name, 9);
	if (!checkdate($this->data[1]->value, $this->data[2]->value, 2000 + $this->data[0]->value))
		$this->result = array('success' => false, 'message' => 'Ung&uuml;ltiges Datum!');
  }


  // Standard-Fall

  if(!$this->result){
  
    for($i=0;$i<count($this->data);$i++){
      $this->pmg .= " ".$this->data[$i]->name;
    }
    $this->pmg = §( isg_phpmapper )->map( $this->pmg );
    
    $em_removed_value = false;
    for($i=0;$i<count($this->data);$i++){
      if( $this->pmg[$i]->presentation == 7 && ($this->data[$i]->value == 36864 || $this->data[$i]->value == -28672) ){
        $this->pms .= " ".$this->data[$i]->name." ".$this->data[$i]->value;
        continue;
      }
      if($this->pmg[$i]->webType==2) $this->data[$i]->value *= 10;
      elseif($this->pmg[$i]->webType==4) $this->data[$i]->value *= 10;
      elseif($this->pmg[$i]->webType==7) $this->data[$i]->value *= 100;
      elseif($this->pmg[$i]->webType==11) $this->data[$i]->value *= 1000;
      $em_visible = §( em )->visible;
      // ALWAYS use !== when comparing "" and 0, b/c ""==0 in php, but ""!==0
      if( ( $this->data[$i]->value !== "" && $this->data[$i]->value>=$this->pmg[$i]->minValue && $this->data[$i]->value<=$this->pmg[$i]->maxValue ) || ( $this->pmg[$i]->presentation == 7 && $this->data[$i]->value == -28672 ) || ( $this->pmg[$i]->presentation == 7 && $this->data[$i]->value == 36864))
      {
        $id = $this->data[$i]->name;
        // we do not want to map this value if emi on
        if (!$em_visible || !§(em)->noSaveValue($id))
          $this->pms .= " " . $id . " " . $this->data[$i]->value;
        else
          $em_removed_value = true;
        if( $em_visible )
        {
          // the EM values - 60xxx
          if( in_array( $id, §( em )->objs ) )
            $energiemanagement_save[$id] = $this->data[$i]->value;
          // the normal set values - hw/room1/2 comfort/eco set, saved especially for EM
          if (§(em)->isEmSpecialValue($this->data[$i])) 
            $EmSpecialValues[$id] = $this->data[$i]->value;
        }
      }
      else
      {
        $this->result = array('success' => false, 'message' => 'Ein ge&auml;nderter Wert entspricht nicht den Vorgaben!');
        return;
      }
    }

    // save these ones even if pms/isg_phpmapper is empty
    if( !empty( $EmSpecialValues ) )
    {
      §( em )->checkSaveEmSetValues( $EmSpecialValues );
    }
    // not empty
    if($this->pms != "-s")
    {
      //print("pms: ".$this->pms."\n<br/>");
      $this->pms = §( isg_phpmapper )->map( $this->pms );
      // while these cannot be non-empty if pms is empty
      if( !empty( $energiemanagement_save ) )
      {
        §( em )->check_save( $energiemanagement_save );
      }
    }
    // if a value was removed b/c of EM, and none left, we don't do any isg_phpmapper, but neither have an error
    else if (!$em_removed_value)
    {
      $this->result = array('success' => false, 'message' => 'Falsche Angaben!');
    }
    
    if(!$this->result){
      $this->result = array('success' => true, 'message' => 'Die Einstellungen wurden erfolgreich gespeichert.');
    }
  }
  }
  
  
  function SpeichernNetzwerk($vals) {
    sc::SpeichernProxyConfig();
    $pfad = $vals[0];
    $modus = 'SSV_ETH0_MODUS="(.*[^\"])"';
    $networklist=array( // Muss die selbe Reihenfolge wie in der Network haben!
      'SSV_ETH0_IP="(.*[^\"])"',
      'SSV_ETH0_NM="(.*[^\"])"',
      'SSV_DGW="(.*[^\"])"',
      'SSV_DNS1="(.*[^\"])"',
      'SSV_DNS2="(.*[^\"])"',
      'SSV_DNS3="(.*[^\"])"'
    );
	 $network=array( // Muss die selbe Reihenfolge wie in der Network haben!
	  'SSV_ETH0_AUTO=',
	  'SSV_ETH0_MODUS=',
      'SSV_ETH0_IP=',
      'SSV_ETH0_NM=',
      'SSV_DGW=',
      'SSV_DNS1=',
      'SSV_DNS2=',
      'SSV_DNS3=',
	  'SSV_NBNS=',
	  'SSV_DGW_ON=',
	  'SSV_DNS_ON='
	);
   
    $networknames=array( // Dazugehoerige Bedeutungsnamen fuer den Client.
      'IP-Adresse',
      'Subnetzmaske',
      'Standardgateway',
      'DNS-Server1',
      'DNS-Server2',
      'DNS-Server3'
    );
    $checks_s=array(
      'SSV_DGW_ON="(.*[^\"])"',
      'SSV_DNS_ON="(.*[^\"])"'
    );
	$networkdata = array();
    $restart_message = '<br /><br />'.§( sprache )->txt( 'txt_HTML_INFOTEXT20' );
    
	/*
    if(!$nc = file_get_contents($pfad) || 1) {
		return json_encode($this->result = array('success' => false, 'message' => 'Datei ip konnte nicht gelesen werden!'));
	}
	*/
	
	$file = $GLOBALS['Netzwerkeinst'];
    // load current values of file
    if( file_exists($file))
    {
		$a = file_get_contents( $file );

		$a = explode( "\n", $a );
		foreach( $a as $l )
		{
			$b = explode( "=", $l );
			$read_objs[$b[0]] = $b[1];

		}

		if($this->data[0]->name=='netzwerk_dhcp')
		{
			$read_objs['SSV_ETH0_MODUS']='"'."dhcp".'"';
			$read_objs['SSV_DGW_ON']='"'.'"';
			$read_objs['SSV_DNS_ON']='"'.'"';
			$read_objs['SSV_NBNS']='"'."servicewelt".'"';
			if( strtoupper( §( sprache )->sprache ) != 'DEUTSCH')
			{
				$this->result = array('success' => true, 'message' => 'DHCP sucessfully activated.');
			}
			else
			{
				$this->result = array('success' => true, 'message' => 'DHCP ist aktiviert!');
			}
			$this->storageResults['dhcp'] = $this->result;
			$this->oktext = true;
    	}
		else
		{
			$feldergefuellt = true;
			//Felder auslesen
			$ip =  '"' . $this->data[0]->value . "." . $this->data[1]->value . "." . $this->data[2]->value . "." . $this->data[3]->value . '"'; 
			$subnet = '"' . $this->data[4]->value . "." . $this->data[5]->value . "." . $this->data[6]->value . "." . $this->data[7]->value . '"';
			$gateway = '"' . $this->data[8]->value . "." . $this->data[9]->value . "." . $this->data[10]->value . "." . $this->data[11]->value . '"';
			$dns1 =  '"' . $this->data[12]->value . "." . $this->data[13]->value . "." . $this->data[14]->value . "." . $this->data[15]->value . '"';
			$dns2 =  '"' . $this->data[16]->value . "." . $this->data[17]->value . "." . $this->data[18]->value . "." . $this->data[19]->value . '"';
			$dns3 =  '"' . $this->data[20]->value . "." . $this->data[21]->value . "." . $this->data[22]->value . "." . $this->data[23]->value . '"';
			
			//Felder überprüfen
			//Ip
			if($this->data[0]->value=="" || $this->data[1]->value=="" || $this->data[2]->value=="" || $this->data[3]->value=="")
			{
			  $feldergefuellt = false;
			  if( strtoupper( §( sprache )->sprache ) != 'DEUTSCH')
			  {
				$this->result = array('success' => false, 'message' => 'Please enter ip.');
			  }
			  else
			  {
				 $this->result = array('success' => false, 'message' => 'Bitte geben Sie eine Ip Adresse ein.');
			  }	
			}
			
			//subnet
			if($this->data[4]->value=="" || $this->data[5]->value=="" || $this->data[6]->value=="" || $this->data[7]->value=="")
			{
			  $feldergefuellt = false;
			  if( strtoupper( §( sprache )->sprache ) != 'DEUTSCH')
			  {
				$this->result = array('success' => false, 'message' => 'Please enter subnetmask.');
			  }
			  else
			  {
				 $this->result = array('success' => false, 'message' => 'Bitte geben Sie eine Subnetzmaske ein.');
			  }	
			}
			
			//gateway
			if($this->data[8]->value=="" || $this->data[9]->value=="" || $this->data[10]->value=="" || $this->data[11]->value=="")
			{
			  $feldergefuellt = false;
			  if( strtoupper( §( sprache )->sprache ) != 'DEUTSCH')
			  {
				$this->result = array('success' => false, 'message' => 'Please enter gateway.');
			  }
			  else
			  {
				 $this->result = array('success' => false, 'message' => 'Bitte geben Sie ein Standardgateway ein.');
			  }	
			}
			
			//dns1
			if($this->data[12]->value=="" || $this->data[13]->value=="" || $this->data[14]->value=="" || $this->data[15]->value=="")
			{
			  $feldergefuellt = false;
			  if( strtoupper( §( sprache )->sprache ) != 'DEUTSCH')
			  {
				$this->result = array('success' => false, 'message' => 'Please enter dns1.');
			  }
			  else
			  {
				 $this->result = array('success' => false, 'message' => 'Bitte geben Sie einen DNS 1 ein.');
			  }	
			}
			
			//dns2
			if($this->data[16]->value!="" || $this->data[17]->value!="" || $this->data[18]->value!="" || $this->data[19]->value!="")
			{
				if($this->data[16]->value=="" || $this->data[17]->value=="" || $this->data[18]->value=="" || $this->data[19]->value=="")
				{
				  $feldergefuellt = false;
				  if( strtoupper( §( sprache )->sprache ) != 'DEUTSCH')
				  {
					$this->result = array('success' => false, 'message' => 'Please enter dns2.');
				  }
				  else
				  {
					 $this->result = array('success' => false, 'message' => 'Bitte geben Sie einen DNS 2 ein.');
				  }	
				}
			}
			//dns3
			if($this->data[20]->value!="" || $this->data[21]->value!="" || $this->data[22]->value!="" || $this->data[23]->value!="")
			{
				if($this->data[20]->value=="" || $this->data[21]->value=="" || $this->data[22]->value=="" || $this->data[23]->value=="")
				{
				  $feldergefuellt = false;
				  if( strtoupper( §( sprache )->sprache ) != 'DEUTSCH')
				  {
					$this->result = array('success' => false, 'message' => 'Please enter dns3.');
				  }
				  else
				  {
					 $this->result = array('success' => false, 'message' => 'Bitte geben Sie einen DNS 3 ein.');
				  }	
				}
			}
			
			if($feldergefuellt)
			{
			  //Werte ändern
			  $read_objs['SSV_ETH0_AUTO']='"on"';
			  $read_objs['SSV_ETH0_MODUS']='"static"';
			  $read_objs['SSV_ETH0_IP'] = $ip;
			  $read_objs['SSV_ETH0_NM']= $subnet;
			  $read_objs['SSV_DGW'] = $gateway;
			  $read_objs['SSV_DNS1']  = $dns1;
			  $read_objs['SSV_DNS2']  = $dns2;
			  $read_objs['SSV_DNS3'] = $dns3;
			  $read_objs['SSV_DGW_ON']='"yes"';
			  $read_objs['SSV_DNS_ON']='"yes"';
			  $read_objs['SSV_NBNS']='"'."servicewelt".'"'; 
			  
			  //sprachabhaengige Meldung
			  if( strtoupper( §( sprache )->sprache ) != 'DEUTSCH')
			  {
				 $this->result = array('success' => true, 'message' => 'static ip sucessfully activated.');
			  }
			  else
			 {
				 $this->result = array('success' => true, 'message' => 'Statische Adressen &uuml;bernommen.'.$stp2);
			 }
			 
			}			 
    	}
	} 
	else 
	{
		$this->result = array('success' => false, 'message' => 'Datei ip konnte nicht gelesen werden!');
	}
	
    if($this->result['success'])
	{
	$write_objs =  $read_objs; 
	
	$file = $GLOBALS['Netzwerkeinst'];
    $write_text = '';
    foreach( $write_objs as $id => $val )
    {
	  if($id!='')
	  {
      $write_text .= $id.'='.$val."\n";
	  }
    }
	
    if(!fd_w( $file, $write_text ))
	{
		$this->result = array('success' => false, 'message' => 'Konnte nicht gespeichert werden!');
	}
      //if(!fd_w($pfad,$nc)) return json_encode($this->result = array('success' => false, 'message' => 'Konnte nicht gespeichert werden!'));
    }
	$this->storageResults['dhcp'] = $this->result;
  }
function SpeichernProxyConfig()
{
	// get index
	if($this->data[24]->name=='netzwerk_proxy') {
		$fieldIndex = 24;
	} elseif($this->data[1]->name=='netzwerk_proxy') {
		$fieldIndex = 1;
	}

	$file = $GLOBALS['portalstatus_pw_datei'];

	if(!file_get_contents($file)) 
	{
		$this->result = array('success' => false, 'message' => 'Datei proxy konnte nicht gelesen werden!');
		//return json_encode($this->result);
	} 
	else 
	{
		// load current values of file
		if( file_exists($file))
		{
			$a = file_get_contents( $file );
	  
			$a = explode( "\n", $a );
			foreach( $a as $l )
			{
				$b = explode( "=", $l );
				$read_content[$b[0]] = $b[1];
			}
		}
	}

    if(isset($fieldIndex))
	{
		$proxyFields = array('benutzer', 'passwort', 'passwortwiederholung', 'proxyserver', 'portnummer');
		$proxyFieldsData = array();
		foreach($proxyFields as $proxyFieldIndex => $proxyFieldValue) {
			$proxyFieldsData[$proxyFieldValue] = $this->data[$fieldIndex + $proxyFieldIndex + 1]->value;
		}

		if($proxyFieldsData['passwort'] == $proxyFieldsData['passwortwiederholung'] ) 
		{
			//Werte ändern
			$read_content['ProxySupport']= 'on'; 
			$read_content['ProxyUser']= $proxyFieldsData['benutzer']; 
			$read_content['ProxyPassword']= $proxyFieldsData['passwort'];
			$read_content['ProxyMachine']= $proxyFieldsData['proxyserver'];
			$read_content['ProxyPort']= $proxyFieldsData['portnummer']; 

			if( strtoupper( §( sprache )->sprache ) != 'DEUTSCH')
			{
					$this->result = array('success' => true, 'message' => 'proxy-configuration sucessfully activated.');
			}
			else
			{
					$this->result = array('success' => true, 'message' => 'Proxy Konfiguration erfolgreich &uuml;bernommen.');
			}	
			
			
			//$read_content['ProxySupport']= 'off';
			if( $read_content['ProxyMachine']=='') 
			{
		   //sprachabhaengige Meldung
				if( strtoupper( §( sprache )->sprache ) != 'DEUTSCH')
				{
					$this->result = array('success' => false, 'message' => 'Please check the proxy-configuration.');
				}
				else
				{
					$this->result = array('success' => false, 'message' => 'Bitte &uuml;berpr&uuml;fen Sie die Proxy-Konfiguration.');
				}		
		    }
        }		
		else 
		{
			$this->result = array('success' => false, 'message' => 'Passwort Wiederholung inkorrekt!');
		}

		$this->storageResults['proxy'] = $this->result;
    }
	else
	{
	    $read_content['ProxySupport']= 'off'; 
		$read_content['ProxyUser']= ''; 
		$read_content['ProxyPassword']= '';
		$read_content['ProxyMachine']= '';
		$read_content['ProxyPort']= '';
		if($this->result['success'] !== false) {
			$this->result['success'] = true;
		}
	}

    if($this->result['success'])
	{
	$write_content =  $read_content; 
	
	$file = $GLOBALS['portalstatus_pw_datei'];
    $text = '';
    foreach( $write_content as $id => $val )
    {
	  if($id!='')
	  {
      $text .= $id.'='.$val."\n";
	  }
    }
	
    fd_w( $file, $text );
	}
}
  
  function SpeichernSicherheit($vals){
    $pfad = $vals[1];
    $dateninhalt = array('Benutzername=','Benutzerpasswort=');
    if(!$daten=file_get_contents($pfad)) return json_encode($this->result = array('success' => false, 'message' => 'Datei konnte nicht gelesen werden!'));
    preg_match('/'.$dateninhalt[0].'(.*[^\\n])/',$daten,$benutzername);
    preg_match('/'.$dateninhalt[1].'(.*[^\\n])/',$daten,$benutzerpasswort);
    if(count($this->data)==4){
      if($this->data[2]->value==$this->data[3]->value&&$this->data[2]->value!=''){
        if($this->data[1]->value!=''){
          $new_daten = $dateninhalt[0].$this->data[1]->value."\n".$dateninhalt[1].$this->data[2]->value;
          $this->result = array('success' => true, 'message' => 'Benuzername und Passwort erfolgreich eingerichtet');
        }else $this->result = array('success' => false, 'message' => 'Sie m&uuml;ssen einen Benutzernamen w&auml;hllen!');
      }else $this->result = array('success' => false, 'message' => 'Passwort Wiederholung inkorrekt!');
    }else{
      if($benutzerpasswort[1]==$this->data[2]->value){
        if($this->data[3]->value==$this->data[4]->value){
          if($this->data[3]->value==''&&$this->data[1]->value==''){
            $this->result = array('success' => true, 'message' => 'Einstellungen wurden zur&uuml;ckgesetzt');
            $new_daten = $dateninhalt[0]."\n".$dateninhalt[1];
            $zurueckgesetzt = true;
          }
          if($this->data[1]->value!=''){
            $new_daten = $dateninhalt[0].$this->data[1]->value."\n".$dateninhalt[1];
            $this->result = array('success' => true, 'message' => 'Benutzername erfolgreich ge&auml;ndert. ');
          }
          if($this->data[3]->value!=''){
            $new_daten.=$this->data[3]->value;
            $this->result['success'] = true;
            $this->result['message'] .= 'Passwort erfolgreich ge&auml;ndert.';
          }elseif(!$zurueckgesetzt){
            $new_daten.=$benutzerpasswort[1];
            if(!$this->result) $this->result = array('success' => true, 'message' => 'Einstellungen beibehalten.');
          }
        }else $this->result = array('success' => false, 'message' => 'Passwort Wiederholung inkorrekt!');
      }else $this->result = array('success' => false, 'message' => 'Altes Passwort inkorrekt!');
    }
    if($this->result['success']){
      if(!fd_w($pfad,$new_daten)) return json_encode($this->result = array('success' => false, 'message' => 'Konnte nicht gespeichert werden!'));
      $daten=file_get_contents($pfad);
      preg_match('/'.$dateninhalt[0].'(.*[^\\n])/',$daten,$benutzername);
      preg_match('/'.$dateninhalt[1].'(.*[^\\n])/',$daten,$benutzerpasswort);
      if($benutzername[1]){
        $_SESSION['user']=$benutzername[1];
        $_SESSION['pass']=$benutzerpasswort[1];
      }else{
        unset($_SESSION['user']);
        unset($_SESSION['pass']);
      }
    }
  }
  
  function SpeichernGeraetebild(){
    include_once('obj/anlagegeraet.php');
    $anlagegeraet = new anlagegeraet( (object) array(
      'bilddocument' => $GLOBALS['web_start_side_pic_path'])
    );
    if( $anlagegeraet->set_bild( $this->data[0]->value ) ){
      $this->result = array('success' => true, 'message' => 'Bild wurde erfolgreich &uuml;bernommen.');
    }else{
      $this->result = array('success' => false, 'message' => 'Bild konnte nicht &uuml;bernommen werden!');
    }
  }
  
  function SpeichernSprache(){
    if( !empty( $this->data[0]->value ) ){
      §( sprache )->set( $this->data[0]->value, '', '' );
      $this->result['success'] = true;
      return;
    }
  }
  
  function SpeichernLandUndSprache(){
    if( !empty( $this->data[0]->value ) ){
			
			$split = explode( " ", $this->data[0]->value );
			$countryName = $split[0];
			
			$file = $GLOBALS['laenderliste'];
			
			if( file_exists($file))
			{
				$fileContent = file_get_contents( $file );
				$fileContent = explode( "\r\n", $fileContent );
				
				foreach( $fileContent as $fileLine )
				{
					$foundPos = strpos($fileLine, $countryName);
					if ($foundPos !== false)
					{
						$split = explode( ";", $fileLine );
						
						$country = $split[0];
						$language = $split[2];
						$countryAndLanguageDisplayName = $split[1] . '(' . $split[2] . ')';
						break;
					}
				}

				§( sprache )->set( $language, $country, $countryAndLanguageDisplayName);
				$this->result['success'] = true;
				return;
			}
			
			$this->result['success'] = false;
			return;
    }
	
	$this->result['success'] = false;
	return;
  }
  
  function output(){
    if( strtoupper( §( sprache )->sprache ) != 'DEUTSCH' || !$this->result['message'] ){
      if( !$this->oktext ){
        /* MEHRSPRACHIGE STANDART MELDUNGEN */
        if( $this->result['warning'] == true ){
          $this->result['message'] = §( sprache )->txt( 'txt_HTML_STANDARD_ANTWORT_WARNUNG' );
        }elseif( $this->result['success'] == true ){
          $this->result['message'] = §( sprache )->txt( 'txt_HTML_STANDARD_ANTWORT_OK' );
        }else{
          $this->result['message'] = §( sprache )->txt( 'txt_HTML_STANDARD_ANTWORT_FEHLER' );
        }
        /***********************************/
      }
    }
    $this->result['message'] = utf8_decode( $this->result['message'] );

//print_r($this->storageResults);
	
	$resultSuccessTrue = array();
	$resultSuccessFalse = array();
	if($this->storageResults) {
		foreach($this->storageResults as $storagedResultIndex => $storageResultValue) {
			if($storageResultValue['success']) {
				$resultSuccessTrue[] = $storageResultValue['message'];
			} else {
				$resultSuccessFalse[] = $storageResultValue['message'];
			}
		}

		if($resultSuccessFalse) {
			$this->result = array(
				'success' => false,
				'message' => implode("<br>\n", $resultSuccessFalse)
			);
		} elseif($resultSuccessTrue) {
			$this->result = array(
				'success' => true,
				'message' => implode("<br>\n", $resultSuccessTrue)
			);
		}
		
	}
		//print_r($this->result);
	
    echo json_encode( $this->result );
  }
}

// MANUELLER DATEI UPLOAD
if($_FILES['manualdataupload']){
  if(is_uploaded_file($_FILES['manualdataupload']['tmp_name']))
  if(substr($_FILES['manualdataupload']['name'],-4)=='.tgz')
  if(move_uploaded_file($_FILES['manualdataupload']['tmp_name'],'../../update/'.$_FILES['manualdataupload']['name'])){
    shell_exec('sync');
    $this->result['success'] = true;
    $this->result['message'] = §( sprache )->txt( 'txt_HTML_STANDARD_ANTWORT_OK' );
    exit();
  }else{
    $this->result['success'] = false;
    $this->result['message'] = §( sprache )->txt( 'txt_HTML_STANDARD_ANTWORT_FEHLER' );
  }
  echo json_encode( $this->result );
  #shell_exec('reboot');
  exit;
}
// Objekt Erzeugung
$sc = new sc;
// Request Empfang
$data = json_decode(stripslashes($_POST['data']), false);

// Datenverarbeitung
$sc->set_data(&$data,array(&$Netzwerkeinst,&$persdaten));
// Ergebnis ausgeben 
$sc->output();
?>
