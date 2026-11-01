
<?php 
$w = 3.6;
$n;
foreach($obj->options as $option){
  $translated_name = §( sprache )->txt( $option->name );
  $str_width = strlen($translated_name)*0.9;
  if($str_width>$w) $w = $str_width;
  if($obj->value == $option->id) $n = $translated_name;
}
if($w>13.5) $w = 13.5;
?>

<div class="calibration round span-24 last" id="calval<?=$obj->webID?>">
  <div class="span-7 ialigned">
    <h3 class="title"><?=§( sprache )->txt( $obj->name )?></h3>
  </div>
  <div class="values span-7">
    <div class="dropdown" style="width:<?=$w*2.1?>em">	<!-- Chrome-Bug: div zu schmal, daher feste Breite. Sorgt ausserdem fuer Platz fuer Radio-Buttons in div.black -->
      <input id="aval<?=$obj->webID?>" value="<?=§( sprache )->txt( $n )?>" readonly="readonly" class="dropdown <?=$w < 5 ? "dropdown_wert" : "dropdown_text"?>" style="width:<?=$w?>em">
      <img src="./pics/icon_edit_dropdown.png" class="dropdown">
      <div class="black">
        <?php foreach($obj->options as $opt) { $translated_name = §( sprache )->txt( $opt->name ); ?>
          <input id="radioval<?=$obj->webID.$opt->id?>" type="radio" alt="<?=$translated_name?>" name="val<?=$obj->webID?>" value="<?=$opt->id?>"<?=($opt->id==$obj->value?' checked="checked"':'')?>>
          <label for="radioval<?=$obj->webID.$opt->id?>"><?=$translated_name?></label>
          <br>
        <?php } ?>
      </div>
    </div>
    
    <script language="javascript" type="text/javascript">
    $(":input[name=val<?=$obj->webID?>]").click( function() {
    if ($("input[name=val<?=$obj->webID?>]:checked").val()==$(this).val()) {
      $("#val<?=$obj->webID?>").val($(this).val());
      $("#aval<?=$obj->webID?>").val(this.alt);
      valueChanged = true;
      closeAllBoxes();
    }});
    </script>

  </div>
  <div class="values span-1 append-1"><?=$obj->unit?></div>
  <div class="green round-right span-8 last">
    <p><?=$obj->info?></p>
  </div>
  <?=§( help_txt_box )->black( $obj->name )?>
</div>