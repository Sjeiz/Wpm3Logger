<?php
$obj->options[0]->name = $EIN  = §( sprache )->txt( 'txt_HTML_EIN_TEXT' );
$obj->options[1]->name = $AUS = §( sprache )->txtu( 'txt_HTML_AUS_TEXT' );
if(strlen($EIN )>$w) $w = strlen($EIN )*0.8;
if(strlen($AUS)>$w) $w = strlen($AUS)*0.8;
$w1 = 2.2 + $w3 = -0.1 + $w2 = $w > 14.9 ? 14.9 : 5; // min | max width
?>

<div class="calibration round span-24 last" id="calval<?=$obj->webID?>" <?=($obj->value?'style="height:140px"':'')?>>
  <div class="span-7 ialigned">
    <h3 class="title"><?=§( sprache )->txt( $obj->name )?></h3>
  </div>
  <div class="values span-7">
    <div class="right" style="height:64px;width:100%">
      <div class="dropdown" style="width:<?=$w1?>em;float:right;">
        <div class="current" style="width:<?=$w2?>em">
          <input id="aval<?=$obj->webID?>" value="<?=$obj->value!=$obj->defaultVal?$EIN:$AUS?>" readonly="readonly" class="dropdown" style="width:<?=$w3?>em margin: 0px; text-align: center; padding: 10px; width: 100%; font-family: Verdana; font-size: 22px;">
        </div>
        <div class="black" style="padding:15px">
          <input id="radioval<?=$obj->webID?>1" type="radio" alt="<?=$EIN?>" onclick="pre7on(<?=$obj->webID?>);" value="1" <?=($obj->value!=$obj->defaultVal?'checked="checked"':'')?>>
          <label for="radioval<?=$obj->webID?>1"><?=$EIN?></label>
          <br />
          <input id="radioval<?=$obj->webID?>0" type="radio" alt="<?=$AUS?>" onclick="pre7off(<?=$obj->webID?>);" value="0" <?=($obj->value!=$obj->defaultVal?'':'checked="checked"')?>>
          <label for="radioval<?=$obj->webID?>0"><?=$AUS?></label>
        </div>
      </div>
    </div>
    <div id="chval<?=$obj->webID?>" style="margin-top:-30px;">
      <div class="editors">
        <a onmousedown="change('val<?=$obj->webID?>','+',<?=$obj->minValue?>,<?=$obj->maxValue?>,'<?=$obj->step?>',true);return false;" href="javascript://">
          <img src="./pics/button_higher.png">
        </a>
	
        <a onmousedown="change('val<?=$obj->webID?>','-',<?=$obj->minValue?>,<?=$obj->maxValue?>,'<?=$obj->step?>',true);return false;" href="javascript://" style="clear:left;">
          <img src="./pics/button_lower.png">
        </a>
      </div>
      <input type="text" tabindex=<?='"'.$obj->tabindex.'"'?> name="val<?=$obj->webID?>" id="val<?=$obj->webID?>" class="edit upndown<? if($obj->firstElement) {?> firstElement<? } if($obj->lastElement) { ?> lastElement<? } ?>" style="width:<?=$obj->size?>em">
    </div>
  </div>
  <div class="values span-1 append-1"><br><div id="valueunit<?=$obj->webID?>"><?=$obj->unit?></div></div>
  <div class="green round-right span-8 last" style="<?=($obj->value?'height:140px':'')?>">
    <p><?=$obj->info?></p>
  </div>
  
  <script language="javascript" type="text/javascript">
    valSettings['val<?=$obj->webID?>'] = new Array();
    valSettings['val<?=$obj->webID?>']['type'] = '<?=$obj->datatype?>'; <? /* is noch von cybay in js notwendig */ ?>
    valSettings['val<?=$obj->webID?>']['min'] = '<?=$obj->minValue?>';
    valSettings['val<?=$obj->webID?>']['max'] = '<?=$obj->maxValue?>';
    jsvalues[<?=$obj->webID?>] = new Array();
    jsvalues[<?=$obj->webID?>]['id']='val<?=$obj->webID?>';
    jsvalues[<?=$obj->webID?>]['val']='<?=( $obj->value == $obj->defaultVal ? $obj->defaultVal : number_format( $obj->value, $obj->steps, ',', '' ) )?>';
	//alert(<?=$obj->value?>);
	pre7<?=($obj->value!=$obj->defaultVal && $obj->value!="-2867.2" && $obj->value!="3686.4" && $obj->value!="32768" ? "on" : 'off')?>(<?=$obj->webID?>,true);
  </script>
</div>
<? } ?>