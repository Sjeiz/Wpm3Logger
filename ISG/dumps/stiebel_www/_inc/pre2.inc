<div class="calibration round span-24 last" id="calval<?=$obj->webID?>">
  <div class="span-7 ialigned">
    <h3 class="title"><?=§( sprache )->txt( $obj->name )?></h3>
  </div>
  <div class="values span-7">
    <div class="info_alone" style="width:<?=$obj->size?>em"><?=!is_numeric($obj->value)?$obj->value:number_format($obj->value,$obj->steps,',','')?></div>
  </div>
  <div class="values span-1 append-1"><?=$obj->unit?></div>
  <div class="green round-right span-8 last">
    <p></p>
  </div>
  <?=§( help_txt_box )->black( $obj->name )?>
</div>
