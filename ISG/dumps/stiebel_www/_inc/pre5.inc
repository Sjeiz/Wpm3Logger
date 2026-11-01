<div class="calibration round span-24 last" id="calval<?=$obj->webID?>">
  <div class="span-7 ialigned">
    <h3 class="title"><?=§( sprache )->txt( $obj->name )?></h3>
  </div>
  <div class="values span-7">
    <div class="editors">
      <a onmousedown="change('val<?=$obj->webID?>','+',<?=$obj->minValue?>,<?=$obj->maxValue?>,'<?=$obj->step?>',true);return false;" href="javascript://">
        <img src="./pics/button_higher.png">
      </a>
      <a id="editorslower" onmousedown="change('val<?=$obj->webID?>','-',<?=$obj->minValue?>,<?=$obj->maxValue?>,'<?=$obj->step?>',true);return false;" href="javascript://" style="clear:left;">
        <img src="./pics/button_lower.png">
      </a>
    </div> 
    <input type="text" tabindex=<?='"'.$obj->tabindex.'"'?> value="" name="val<?=$obj->webID?>" id="val<?=$obj->webID?>" class="edit upndown<? if($obj->firstElement) {?> firstElement<? } if($obj->lastElement) { ?> lastElement<? } ?>" style="width:<?=$obj->size?>em">
	
    <script language="javascript" type="text/javascript">
      valSettings['val<?=$obj->webID?>'] = new Array();
      valSettings['val<?=$obj->webID?>']['type'] = '<?=$obj->datatype?>'; <? /* is noch von cybay in js notwendig */ ?>
      valSettings['val<?=$obj->webID?>']['min'] = '<?=$obj->minValue?>';
      valSettings['val<?=$obj->webID?>']['max'] = '<?=$obj->maxValue?>';
      
      <?php /* $i=0;  if( $obj->step > 0 ) while( $obj->step*pow(10, $i) < 1 ) $i++; */ ?>
      
      jsvalues['<?=$obj->webID?>'] = new Array();
      jsvalues['<?=$obj->webID?>']['id']='val<?=$obj->webID?>';
      jsvalues['<?=$obj->webID?>']['val']='<?=number_format($obj->value,$obj->steps,',','')?>';
    </script>
   </div>
  <div class="values span-1 append-1"><?=$obj->unit?></div>
  <div class="green round-right span-8 last">
    <p><?=$obj->info?></p>
  </div>
  <?=§( help_txt_box )->black( $obj->name )?>
</div>