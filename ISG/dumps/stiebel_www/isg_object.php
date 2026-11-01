<?php
class isg extends sf {
  
  public $ga = array(0);              // Seiten Index
  public $pt = 3;                     // Seitentyp / Page Type
  public $pm = array();               // Php_Mapper
  public $at;                         // Anlagentyp (WP, LWZ, usw)
  public $wpm_warmwasser = false;     // if true = warmwasser ueberall ausblenden
  
  // Links
  public $BildDatenLink = null,
         $LizenzbestimmungLink = null,
         $FehlerlisteLink = null,
         $SpracheLink = null;
  
  private $dependencywebids = array();
  
  function __construct() {
    // ausnahmeregelung -> Rainer Richtsmeier
    $this->wpm_warmwasser = ( $GLOBALS['controllertype'] == $GLOBALS['controllertypes'][0] ? $this->dependencywebidcheck( 24 ) : false );
    // seiten navigations schluessel
    if( !empty( $_GET['s'] ) ){
      $this->ga = explode( ",", $_GET['s'] );
    }
  }
  
  
  
/************************************************************************************************************/
  
  
  
  public function start(){
    // Json Menuestruktur einlesen, Startknoten finden
    $this->json_file();
    // Standartverlinkungen setzen
    $this->set_links( $this->jd );
    // Seitentyp ermitteln
    $this->set_pt();
	if ($_GET["forcepage"]=="dsgvo"){
		$this->set_pt_force(30);
	}

  }
  
  private function json_file() {
    // Menue Struktur einlesen
    $this->jd = json_decode( file_get_contents( $GLOBALS['json_template'] ), false );
    // Anlagentyp filtern
    $this->at = $this->jd->children[2]->children[0]->name;
    // zum Startknoten des Json Objekt
    $this->jd = &$this->jd->children[2]->children[0]->children;
  }
  
   private function set_pt(){
    // bei Erstverwendung des Geraetes, auf Spracheinstellungsseite verweisen
    if( §( sprache )->zur_spracheinstellung_weiterleiten === true )
      if( $this->SpracheLink !== null )
        $this->ga = $this->SpracheLink;
    // Seitentyp setzen
    if(count($this->ga)==1) $this->pt = $this->jd[$this->ga[0]]->pageType;
    else $this->pt = $this->jd[$this->ga[0]]->children[$this->ga[1]]->pageType;
  }

   function set_pt_force(){
	   // var_dump($this->jd);
	   //echo "start";
    // bei Erstverwendung des Geraetes, auf Spracheinstellungsseite verweisen
        $this->ga = $this->DSGVOLink;
    // Seitentyp setzen
if(count($this->ga)==1) $this->pt = $this->jd[$this->ga[0]]->pageType;
        else $this->pt = $this->jd[$this->ga[0]]->children[$this->ga[1]]->pageType;
  }
  
  private function set_links( &$arrays, $ebene=null ){
    // Links zu bestimmten seiten filtern nach Page Type
    $i = 0;
    $j = count($ebene);
    foreach( $arrays as $objekt ){
      $ebene[$j] = $i;
      switch( $objekt->pageType ){
        case 11: $this->BildDatenLink = $ebene; break;
        case 12: $this->FehlerlisteLink = $ebene; break;
        case 13: $this->SpracheLink = $ebene; break;
        case 22: $this->LizenzbestimmungLink = $ebene; break;
        case 30: $this->DSGVOLink = $ebene; break;

      }
      if( $objekt->children ){
        $this::set_links( $objekt->children, $ebene );
      }
      $i++;
    }
  }
  
  function dependencywebidcheck( $webid ){
    // true = ausblenden
    if( $webid <= 0 ){
      return;
    }
    if( !$this->dependencywebids[ $webid ] ){
      $dit = §( isg_phpmapper )->map( '-g '.$webid );
      $this->dependencywebids[ $webid ] = $dit[0];
    }
    if( $this->dependencywebids[ $webid ]->value == 32768 ){
      return true;
    }
  }

  function version(){
    $dit = §( isg_phpmapper )->map( '-g 60001 60002 60003' );
    return 'v'.$dit[0]->value.'.'.$dit[1]->value.'.'.$dit[2]->value;
  }
  

  
/************************************************************************************************************/


  
  function nav_main(){
    // Hauptnavigation erstellen
    include_once('_inc/nav_main.inc');
  }
  function nav_sub(){
    // Unternavigation erstellen
	  include_once('_inc/nav_sub.inc');
  }
  function get_values(){
    // Werte Fuer den php_mapper aufbereiten
    foreach( $this->ga as $index )
      $this->jd = &$this->jd[ $index ]->children;
    if($this->pt==8){
      $this->pm[] = $this->jd[0]->webID;
      $this->pmn[] = $this->jd[0]->name;
    /*
    }elseif($this->pt==7){
      for($i=0;$i<(count($this->jd)/7);$i++){
        $this->pm .= " ".$this->jd[($this->day*(count($this->jd)/7)+$i)]->webID;
        $this->pmn .= $this->jd[($this->day*(count($this->jd)/7)+$i)]->name."[";
      }
    */
    }else{
      for($i=0;$i<count($this->jd);$i++){
        if( $this->dependencywebidcheck( $this->jd[$i]->dependency ) ){
          continue;
        }
        
        if($this->jd[$i]->webID){
          $this->pm[] = $this->jd[$i]->webID;
          $this->pmn[] = $this->jd[$i]->name;
          if($this->jd[$i]->options){
            foreach( $this->jd[$i]->options as $options ){
              if( $this->wpm_warmwasser === true && strtoupper( $options[1] ) == 'WARMWASSERBETRIEB' ){
                continue; // ausnahme
              }
              $this->pmo[$this->jd[$i]->webID][] = (object) array( "id" => $options[0], "name" => $options[1] );
            }
          }
        }else{
          if($this->pt==4){
            $this->pmt[$i]->name = $this->jd[$i]->name; 
            $this->pmt[$i]->anz = count($this->jd[$i]->children);
          }
          for($j=0;$j<count($this->jd[$i]->children);$j++){
            if( $this->dependencywebidcheck( $this->jd[$i]->children[$j]->dependency ) ){
              if( $this->pt == 4 ){
                $this->pmt[$i]->anz--;
              }
              continue;
            }
            $this->pm[] = $this->jd[$i]->children[$j]->webID;
            $this->pmn[] = $this->jd[$i]->children[$j]->name;
            if( $this->jd[$i]->children[$j]->options ){
              foreach( $this->jd[$i]->children[$j]->options as $options ){
                $this->pmo[$this->jd[$i]->children[$j]->webID][] = (object) array( "id" => $options[0], "name" => $options[1] );
              }
            }
          }
        }
      }
      if( !empty( $this->pmt ) ) $this->pmt = array_values( $this->pmt );
    }
    
    if(!empty($this->pmn)){
      // Daten an den php_mapper uebergeben
      $this->pm = §( isg_phpmapper )->map( '-g '.implode( ' ', $this->pm ) );
      // when using EM, and EM is currently "on" (i.e., elevated set values are used), the set values for temp day/night hk1,2 and hot water
      // cannot be used from shared mem/control, 
      // Daten nach dem php_mapper Seitengerecht aufbereiten
      if (§(em)->visible && §(em)->isEmOn())
        §(em)->checkLoadEmSetValues($this->pm);
      for($i=0;$i<count($this->pm);$i++){
        if( isset( $this->pmo[ $this->pm[$i]->webID ] ) ){
          $this->pm[$i]->options = $this->pmo[$this->pm[$i]->webID];
        }
        
        $this->pm[$i]->name = &$this->pmn[$i];
        sf::normalize(&$this->pm[$i]->minValue,&$this->pm[$i]->webType);
        sf::normalize(&$this->pm[$i]->maxValue,&$this->pm[$i]->webType);
        sf::normalize(&$this->pm[$i]->step,&$this->pm[$i]->webType);
        
        if( !in_array( abs( $this->pm[$i]->value ), array( 32768, 3276832768 ) ) ){
          sf::normalize(&$this->pm[$i]->value,&$this->pm[$i]->webType);
        }else{
          if( $this->pm[$i]->presentation != 7 ){
            $this->pm[$i]->value = '-';
          }else{
            $this->pm[$i]->value = abs( $this->pm[$i]->value );
          }
        }
        
        utf8( &$this->pm[$i]->unit ); // <- utf8 Funktion in stablevalues.php
        $this::values_info($i);
        switch( $this->pm[$i]->webType ){
          case 2:  $this->pm[$i]->steps = strlen( $this->wts2  ) -1; break;
          case 4:  $this->pm[$i]->steps = strlen( $this->wts4  ) -1; break;
          case 7:  $this->pm[$i]->steps = strlen( $this->wts7  ) -1; break;
          case 11: $this->pm[$i]->steps = strlen( $this->wts11 ) -1; break;
        }
      }
      unset($this->pmn);  // wird ab hier nicht mehr gebraucht
      unset($this->pmo);  // wird ab hier nicht mehr gebraucht
    }elseif(in_array($this->pt,array(11,12,13,14,20,21,22))){
    }else{
     #echo "Einstellungen noch nicht verf&uuml;gbar.";
    }
  }
  
  function values_info($i){
    $obj =& $this->pm[$i];
    $ref =& §( sprache );
    if($obj->readOnly==1){
      $obj->info = '';
      return;
    }
    switch( $obj->presentation ){
     case 4:
      $obj->info = $ref->txt( 'txt_HTML_OPTIONALE_AUSWAHL' );
      break;
     default:
      $unt =& $ref->txt( $obj->unit );
      $obj->info  = $ref->txt( 'txt_HTML_KETTENTEXT2_TEXT1' ).'<br />';
      $obj->info .= ( is_array( $obj->minValue ) ? str_pad( $obj->minValue[0], 2, '0', STR_PAD_LEFT ).':'.str_pad( $obj->minValue[1], 2, '0', STR_PAD_LEFT ) : $obj->minValue );
      $obj->info .= " $unt ";
      $obj->info .= $ref->txt( 'txt_HTML_KETTENTEXT2_TEXT2' ).' ';
      $obj->info .= ( is_array( $obj->maxValue ) ? str_pad( $obj->maxValue[0], 2, '0', STR_PAD_LEFT ).':'.str_pad( $obj->maxValue[1], 2, '0', STR_PAD_LEFT ) : $obj->maxValue );
      $obj->info .= " $unt";
    }
    $this->ro = true;
  }
}
