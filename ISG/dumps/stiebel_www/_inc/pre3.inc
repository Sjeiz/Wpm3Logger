<div class="bool">
  <div class="right">
    <input type="radio" id="radioval<?=$obj->webID?>off" class="info" value="0" name="val<?=$obj->webID?>" <?=($obj->value?'':'checked="checked"')?>>
    <label class="bool off<?=($obj->value?'off':'on')?>" for="radioval<?=$obj->webID?>off"></label>
  </div>
  <div class="right">
    <input type="radio" id="radioval<?=$obj->webID?>on"  class="info" value="1" name="val<?=$obj->webID?>" <?=($obj->value?'checked="checked"':'')?>>
    <label class="bool on<?=($obj->value?'on':'off')?>" for="radioval<?=$obj->webID?>on"></label>
  </div>
</div>