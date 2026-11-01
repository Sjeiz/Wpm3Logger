<?php
// PHP EINSTELLUNGEN
error_reporting(0);

// PFAD EINSTELLUNGEN
$isg_sendinitdata             = '/var/volatile/firmware/etc/init.d/isg_sendinit.sh';
$isg_send_error_confirmation  = '/var/volatile/firmware/etc/init.d/isg_senderrorconfirmation.sh';
$portal_passwort_auth         = '/var/cache/portal-response-getRequest';
$controllertype               = '/var/cache/controllertype_';
$hp_error_active              = '/firmware/log/hp_error_active';
$portal_user_data             = '/firmware/config/portal-user-data';
$web_start_side_pic_path      = '/firmware/config/webstartsidepicpath';
$main_nav_link                = '.';
$portal_connection_vorgaben   = './ext/portal-connection-vorgaben';
$datenschutzerklaerung        = './doc/';
$laenderliste                 = './doc/laenderliste.csv';
$fehlerliste_index            = './doc/fehlerliste_index.csv';
$anlagen_bilder_auswahl       = './pics/anlagen';
$trigger_button               = '../bin/trigger_button';
$isg_phpmapper                = '../bin/isg_phpmapper';
$json_template                = '/var/volatile/cache/config.json.template';
$portalstatus_pw_datei        = '../../config/libisgclient.cfg';
$persdaten                    = '../../config/persdata';
$Netzwerkeinst                = '../../config/network';
$portal_using                 = '../../config/portal-using';
$energie_management_obj_datei = '../../config/energie_management_object_values';
$Aussentemp                   = '../../log/dashboard.week.log';
$marke                        = '../../../var/cache/firmware-manufacturer';
$portal_last_access           = '../../../var/cache/portal-last-access';
$can_connect_fault            = '../../../var/cache/can-connect-fault';
$network_connect_timeout      = '../../../var/cache/network-connect-timeout';
$pdf_bedienungsanleitungen    = './doc/bedienungsanleitungen';
$SHM_last_access              = '../../../var/cache/SHM-last-access';
$EM_saved_set_values	      = '../../config/saved_set_values';
$EM_on_off_file               = '/var/cache/dashboard.em_on.log';
$EM_energy_file               = '/firmware/log/dashboard.em_energy.log';
	

// VERLAUFSBESTIMMUNGEN
$anmelde_formular_daten = array('txt_HTML_NACHNAME','txt_HTML_VORNAME',array('txt_HTML_STRASSE','txt_HTML_HAUS_NR'),array('txt_HTML_WOHNORT','txt_HTML_PLZ'),'txt_HTML_LAND','txt_HTML_E_MAIL_ADRESSE','txt_HTML_TELEFONNUMMER','txt_HTML_GERAETETYP','txt_HTML_GERAETENUMMER');
$controllertypes            = array('WPM','LWZ');
$reboottimer                = 50; // Sekunden
$dashboard_week_log         = array('steps'=>array(2,5,10,25),'steps_multi'=>2,'max_lines'=>10);
date_default_timezone_set(file_exists($portal_using)?"Europe/Berlin":'UTC');
// historischer Code, 'MAXIMALTEMPERATUR' kommt nirgends mehr vor
// $sonderlocke_namenskopplung = array( 'min' => 'MAXIMALTEMPERATUR', 'max' => 'SOCKELTEMPERATUR' );

// VARIABLENVORBEREITUNGEN
$controllertype             = (file_exists($controllertype.$controllertypes[0])?$controllertypes[0]:$controllertypes[1]);
$marke                      = (trim(file_get_contents($marke))=='tec'?'_tec':'');
$portal_key                 = (preg_match('/^IsgPassword=(.*)/',file_get_contents($portalstatus_pw_datei),$da)?(trim($da[1])!=""?true:false):false);
$portal_last_access         = ( file_exists( $portal_last_access ) ? explode( "\n", file_get_contents( $portal_last_access ) ) : false );
$can_connect_fault          = file_exists($can_connect_fault);
$datenschutzerklaerung     .= (§(sprache)->sprache=='DEUTSCH'?'de':'gb').'_'.($marke?'tec_':'').'portal_datenschutzerklaerung.txt';
$tabindex=0;

//if(!$portal_last_access) $portal_last_access['timestamp'] = time();

// FUNKTIONEN
function fd_w($pfad,$text,$mod="w") { // file Data write
  file_put_contents( $pfad, $text );
  shell_exec('sync');
  return true;
}

function utf8( $text ){
  while( utf8_decode( $text ) != $text ) $text = utf8_decode( $text );
  return $text = utf8_encode( $text );
}

// SPEZIALFUNKTIONEN
function cre_func($name=false,$variablen=false,$inhalt=false){  # INHALT = HTML
  if(function_exists($name)) return true;
  if(is_string($name)&&is_string($variablen)&&is_string($inhalt)){
    eval("function $name($variablen) { ?> $inhalt <?php } ");
    return true;
  }
  return false;
}

function cre_file_func($file){
  if(file_exists($file)){
    $info = pathinfo($file);
    $name = basename($file,(strpos($info['basename'],'.')?'.'.$info['extension']:'')); // auch ohne Dateiendung
    if(cre_func($name,'$obj',file_get_contents($file))) return true;
  }
  return false;
}

// OBJEKTE
class sf { // Standartfunktionen
  // Web Type Step
  protected $wts2 = 10;    // 1 nachkommastelle
  protected $wts4 = 10;
  protected $wts7 = 100;   // 2 nachkommastellen
  protected $wts11 = 1000; // 3 nachkommastellen
  // Werte zur Ausgabe anpassen
  function normalize($value,$type=0){
    switch($type){
      case 1: break;  // frei
      case 2: $value = (string)($value/$this->wts2); break;
      case 3: break;  // frei
      case 4: $value = (string)($value/$this->wts4); break;
      case 5: {
        $startValue = floor($value / 256);
  		  $stopValue =  $value % 256;
        
        $value = array($startValue,$stopValue);
				break; }
      case 6: break;  // frei
      case 7: $value = (string)($value/$this->wts7); break;
      case 8: break;  // frei
      case 9: {
        $hour = floor($value / 4);
  		  $min  = ($value % 4)*15;
        
        $value = array($hour,$min);
        break; }
      case 10: {
        $stopValue = floor($value / 256);
  		  $startValue =  $value % 256;
        
        $hourStart = floor($startValue/4);
  		  $minStart = ($startValue % 4)*15;
        
  		  $hourStop = floor($stopValue/4);
  		  $minStop = ($stopValue % 4)*15;
        
        $value = array($hourStart,$minStart,$hourStop,$minStop);
        break; }
      case 11: $value = (string)($value/$this->wts11); break;	
	  case 15: break; // Verdichterstarts
     default: if(gettype($value)!="array") $value = (string)$value;
    }
    return $value;
  }
}
  
function §($n){ // 02.02.2012
  /**
   *  $n = Der Name des Objects mit dem gearbeitet werden soll
   *  $o = Die Objectdatenbank in der alle Instanzen abgelegt werden
   *  $o[$n] = objekt[name]
   *
   *  Funktions Ziel:
   *  Automatische includierung der Objectdateien : Aufwandserleichterung
   *  Ermöglichung der Objekttunnelung            : Aufwandserleichterung
   *  Vermeidung extensions in Objekten           : Arbeitsspeicher minimieren
   *  Vermeidung doppelter Instanzen              : Arbeitsspeicher minimieren
   *
   *  Status     : global
   *
   *  Anwendungs Beispiel 1 : $sprachen = §( sprache )->sprachen;
   *  Anwendungs Beispiel 2 : §( sprache )->set( 'Deutsch' ); 
   *  Anwendungs Beispiel 3 : $( xy )->get( §( xz )->val );
   */
  static$o;include_once("obj/$n.php");if(!$o[$n])$o[$n]=new$n;return$o[$n];
}

?>
