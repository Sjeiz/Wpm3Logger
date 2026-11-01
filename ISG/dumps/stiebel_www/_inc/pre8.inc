<div class="span-24 slider">
  <div id="<?=$obj->webID?>" class="SliderBoxHeadline round-top">
    <h2 class="curdef" style="text-align:center;padding:5px; color:white;"><?=$obj->wochentag?></h2>
  </div>
  <div>
    <div id="slider<?=$obj->webID?>" style="margin:15px 25px 0;"></div>
    <img src="./pics/slider/<?=($GLOBALS['marke']?'tec-':'')?>zeitleiste.gif" width="901" style="margin:-6px 0 3px 23px" />
  </div>
  <?php foreach( $obj->webIDs as &$i ){ ?>
  <input id="<?=$i?>" type="hidden" value="0;0" name="<?=$i?>">
  <? } ?>
</div>

<script type="text/javascript">
$(function(){
  $( "#slider<?=$obj->webID?>" ).slider({
    range: true,
    min: 0,
    max: 96,
    values: [<?=implode(',',$obj->values)?>],
    
    ranges: true,
    pushSlider: true,
    slideGroup: true,
    dblchange: true,
    ids: ["<?=implode('","',$obj->webIDs)?>"],
    puffer: [<?=implode(',',$obj->values)?>],
    defaultValue: <?=$obj->defaultValue?>,
    
    isChange: function( values ){
      if( $("#"+this.id.replace("slider","")).hasClass("SliderBoxHeadline-active" ) ){
        var self = this;
        $("div.SliderBoxHeadline-active").each(function(){
          if( self.id != "slider"+this.id ){
            $( "#slider"+this.id ).slider( "resetSlider", $(self).slider("values") );
          }
        });
      }
    }
    
  });
});
</script>