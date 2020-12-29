/* *
 *
 *  WebAudio-Controls is based on
 *    webaudio-knob by Eiji Kitamura http://google.com/+agektmr
 *    webaudio-slider by RYoya Kawai https://plus.google.com/108242669191458983485/posts
 *    webaudio-switch by Keisuke Ai http://d.hatena.ne.jp/aike/
 *  Integrated and enhanced by g200kg http://www.g200kg.com/
 *
 *	Copyright 2013 Eiji Kitamura / Ryoya KAWAI / Keisuke Ai / g200kg(Tatsuya Shinyagaito)
 *
 *	 Licensed under the Apache License, Version 2.0 (the "License");
 *	 you may not use this file except in compliance with the License.
 *	 You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 *	 Unless required by applicable law or agreed to in writing, software
 *	 distributed under the License is distributed on an "AS IS" BASIS,
 *	 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	 See the License for the specific language governing permissions and
 *	 limitations under the License.
 *
 * */
if(window.customElements){
  let styles=document.createElement("style");
  styles.innerHTML=
`#webaudioctrl-context-menu {
  display: none;
  position: absolute;
  z-index: 10;
  padding: 0;
  width: 100px;
  color:#eee;
  background-color: #268;
  border: solid 1px #888;
  box-shadow: 1px 1px 2px #888;
  font-family: sans-serif;
  font-size: 11px;
  line-height:1.7em;
  text-align:center;
  cursor:pointer;
  color:#fff;
  list-style: none;
}
#webaudioctrl-context-menu.active {
  display: block;
}
.webaudioctrl-context-menu__item {
  display: block;
  margin: 0;
  padding: 0;
  color: #000;
  background-color:#eee;
  text-decoration: none;
}
.webaudioctrl-context-menu__title{
  font-weight:bold;
}
.webaudioctrl-context-menu__item:last-child {
  margin-bottom: 0;
}
.webaudioctrl-context-menu__item:hover {
  background-color: #b8b8b8;
}
`;
  document.head.appendChild(styles);
  let midimenu=document.createElement("ul");
  midimenu.id="webaudioctrl-context-menu";
  midimenu.innerHTML=
`<li class="webaudioctrl-context-menu__title">MIDI Learn</li>
<li class="webaudioctrl-context-menu__item" id="webaudioctrl-context-menu-learn" onclick="webAudioControlsMidiManager.contextMenuLearn()">Learn</li>
<li class="webaudioctrl-context-menu__item" onclick="webAudioControlsMidiManager.contextMenuClear()">Clear</li>
<li class="webaudioctrl-context-menu__item" onclick="webAudioControlsMidiManager.contextMenuClose()">Close</li>
`;
  let opt={
    useMidi:0,
    midilearn:0,
    mididump:0,
    outline:0,
    knobSrc:null,
    knobSprites:0,
    knobWidth:0,
    knobHeight:0,
    knobDiameter:64,
    knobColors:"#e00;#000;#000",
    sliderSrc:null,
    sliderKnobsrc:null,
    sliderWidth:0,
    sliderHeight:0,
    sliderKnobwidth:0,
    sliderKnobheight:0,
    sliderDitchlength:0,
    sliderColors:"#e00;#000;#fcc",
    switchWidth:0,
    switchHeight:0,
    switchDiameter:24,
    switchColors:"#e00;#000;#fcc",
    paramWidth:32,
    paramHeight:16,
    paramColors:"#fff;#000",
    valuetip:0,
    xypadColors:"#e00;#000;#fcc",
  };
  if(window.WebAudioControlsOptions)
    Object.assign(opt,window.WebAudioControlsOptions);
  class WebAudioControlsWidget extends HTMLElement{
    constructor(){
      super();
      this.addEventListener("keydown",this.keydown);
      this.addEventListener("mousedown",this.pointerdown,{passive:false});
      this.addEventListener("touchstart",this.pointerdown,{passive:false});
      this.addEventListener("wheel",this.wheel,{passive:false});
      this.addEventListener("mouseover",this.pointerover);
      this.addEventListener("mouseout",this.pointerout);
      this.addEventListener("contextmenu",this.contextMenu);
      this.hover=this.drag=0;
      document.body.appendChild(midimenu);
      this.basestyle=`
.webaudioctrl-tooltip{
  display:inline-block;
  position:absolute;
  margin:0 -1000px;
  z-index: 999;
  background:#eee;
  color:#000;
  border:1px solid #666;
  border-radius:4px;
  padding:5px 10px;
  text-align:center;
  left:0; top:0;
  font-size:11px;
  opacity:0;
  visibility:hidden;
}
.webaudioctrl-tooltip:before{
  content: "";
  position: absolute;
  top: 100%;
  left: 50%;
  margin-left: -8px;
  border: 8px solid transparent;
  border-top: 8px solid #666;
}
.webaudioctrl-tooltip:after{
  content: "";
  position: absolute;
  top: 100%;
  left: 50%;
  margin-left: -6px;
  border: 6px solid transparent;
  border-top: 6px solid #eee;
}
`;
    }
    sendEvent(ev){
      let event;
      event=document.createEvent("HTMLEvents");
      event.initEvent(ev,false,true);
      this.dispatchEvent(event);
    }
    getAttr(n,def){
      let v=this.getAttribute(n);
      if(v==""||v==null) return def;
      switch(typeof(def)){
      case "number":
        if(v=="true") return 1;
        v=+v;
        if(isNaN(v)) return 0;
        return v;
      }
      return v;
    }
    showtip(d){
      function valstr(x,c,type){
        switch(type){
        case "x": return (x|0).toString(16);
        case "X": return (x|0).toString(16).toUpperCase();
        case "d": return (x|0).toString();
        case "f": return x.toFixed(c);
        case "s": return x.toString();
        }
        return "";
      }
      function numformat(s,x){
        if(typeof(x)=="undefined")
          return;
        let i=s.indexOf("%");
        let c=[0,0],type=0,m=0,r="",j=i+1;
        for(;j<s.length;++j){
          if("dfxXs".indexOf(s[j])>=0){
            type=s[j];
            break;
          }
          if(s[j]==".")
            m=1;
          else
            c[m]=c[m]*10+parseInt(s[j]);
        }
        if(typeof(x)=="number")
          r=valstr(x,c[1],type);
        else
          r=valstr(x.x,c[1],type)+","+valstr(x.y,c[1],type);
        if(c[0]>0)
          r=("               "+r).slice(-c[0]);
        r=s.replace(/%.*[xXdfs]/,r);
        return r;
      }
      let s=this.tooltip;
      if(this.drag||this.hover){
        if(this.valuetip){
          if(s==null)
            s=`%.${this.digits}f`;
          else if(s.indexOf("%")<0)
            s+=` : %.${this.digits}f`;
        }
        if(s){
          this.ttframe.innerHTML=numformat(s,this.convValue);
          this.ttframe.style.display="inline-block";
          this.ttframe.style.width="auto";
          this.ttframe.style.height="auto";
          this.ttframe.style.transition="opacity 0.5s "+d+"s,visibility 0.5s "+d+"s";
          this.ttframe.style.opacity=0.9;
          this.ttframe.style.visibility="visible";
          let rc=this.getBoundingClientRect(),rc2=this.ttframe.getBoundingClientRect(),rc3=document.documentElement.getBoundingClientRect();
          this.ttframe.style.left=((rc.width-rc2.width)*0.5+1000)+"px";
          this.ttframe.style.top=(-rc2.height-8)+"px";
          return;
        }
      }
      this.ttframe.style.transition="opacity 0.1s "+d+"s,visibility 0.1s "+d+"s";
      this.ttframe.style.opacity=0;
      this.ttframe.style.visibility="hidden";
    }
    pointerover(e) {
      this.hover=1;
      this.showtip(0.6);
    }
    pointerout(e) {
      this.hover=0;
      this.showtip(0);
    }
    contextMenu(e){
      if(window.webAudioControlsMidiManager && this.midilearn)
        webAudioControlsMidiManager.contextMenuOpen(e,this);
      e.preventDefault();
      e.stopPropagation();
    }
    setMidiController(channel, cc) {
      if (this.listeningToThisMidiController(channel, cc)) return;
      this.midiController={ 'channel': channel, 'cc': cc};
      console.log("Added mapping for channel=" + channel + " cc=" + cc + " tooltip=" + this.tooltip);
    }
    listeningToThisMidiController(channel, cc) {
      const c = this.midiController;
      if((c.channel === channel || c.channel < 0) && c.cc === cc)
        return true;
      return false;
    }
    processMidiEvent(event){
      const channel = event.data[0] & 0xf;
      const controlNumber = event.data[1];
      if(this.midiMode == 'learn') {
        this.setMidiController(channel, controlNumber);
        webAudioControlsMidiManager.contextMenuClose();
        this.midiMode = 'normal';
      }
      if(this.listeningToThisMidiController(channel, controlNumber)) {
        if(this.tagName=="WEBAUDIO-SWITCH"){
          switch(this.type){
          case "toggle":
            if(event.data[2]>=64)
              this.setValue(1-this.value,true);
            break;
          case "kick":
            this.setValue(event.data[2]>=64?1:0);
            break;
          case "radio":
            let els=document.querySelectorAll("webaudio-switch[type='radio'][group='"+this.group+"']");
            for(let i=0;i<els.length;++i){
              if(els[i]==this)
                els[i].setValue(1);
              else
                els[i].setValue(0);
            }
            break;
          }
        }
        else{
          const val = this.min+(this.max-this.min)*event.data[2]/127;
          this.setValue(val, true);
        }
      }
    }
  }

try{
    customElements.define("webaudio-knob", class WebAudioKnob extends WebAudioControlsWidget {
    constructor(){
      super();
    }
    connectedCallback(){
      let root;
//      if(this.attachShadow)
//        root=this.attachShadow({mode: 'open'});
//      else
        root=this;
      root.innerHTML=
`<style>
${this.basestyle}
webaudio-knob{
  display:inline-block;
  position:relative;
  margin:0;
  padding:0;
  cursor:pointer;
  font-family: sans-serif;
  font-size: 11px;
}
.webaudio-knob-body{
  display:inline-block;
  position:relative;
  margin:0;
  padding:0;
  vertical-align:bottom;
}
</style>
<div class='webaudio-knob-body' tabindex='1' touch-action='none'></div><div class='webaudioctrl-tooltip'></div>
`;
      this.elem=root.childNodes[2];
      this.ttframe=root.childNodes[3];
      this.enable=this.getAttr("enable",1);
      this._src=this.getAttr("src",opt.knobSrc); Object.defineProperty(this,"src",{get:()=>{return this._src},set:(v)=>{this._src=v;this.setupImage()}});
      this._value=this.getAttr("value",0); Object.defineProperty(this,"value",{get:()=>{return this._value},set:(v)=>{this._value=v;this.redraw()}});
      this.defvalue=this.getAttr("defvalue",0);
      this._min=this.getAttr("min",0); Object.defineProperty(this,"min",{get:()=>{return this._min},set:(v)=>{this._min=+v;this.redraw()}});
      this._max=this.getAttr("max",100); Object.defineProperty(this,"max",{get:()=>{return this._max},set:(v)=>{this._max=+v;this.redraw()}});
      this._step=this.getAttr("step",1); Object.defineProperty(this,"step",{get:()=>{return this._step},set:(v)=>{this._step=+v;this.redraw()}});
      this._sprites=this.getAttr("sprites",opt.knobSprites); Object.defineProperty(this,"sprites",{get:()=>{return this._sprites},set:(v)=>{this._sprites=v;this.setupImage()}});
      this._width=this.getAttr("width",opt.knobWidth); Object.defineProperty(this,"width",{get:()=>{return this._width},set:(v)=>{this._width=v;this.setupImage()}});
      this._height=this.getAttr("height",opt.knobHeight); Object.defineProperty(this,"height",{get:()=>{return this._height},set:(v)=>{this._height=v;this.setupImage()}});
      this._diameter=this.getAttr("diameter",opt.knobDiameter); Object.defineProperty(this,"diameter",{get:()=>{return this._diameter},set:(v)=>{this._diameter=v;this.setupImage()}});
      this._colors=this.getAttr("colors",opt.knobColors); Object.defineProperty(this,"colors",{get:()=>{return this._colors},set:(v)=>{this._colors=v;this.setupImage()}});
      this.outline=this.getAttr("outline",opt.outline);
      this.sensitivity=this.getAttr("sensitivity",1);
      this.valuetip=this.getAttr("valuetip",opt.valuetip);
      this.tooltip=this.getAttr("tooltip",null);
      this.conv=this.getAttr("conv",null);
      if(this.conv){
        const x=this._value;
        this.convValue=eval(this.conv);
        if(typeof(this.convValue)=="function")
          this.convValue=this.convValue(x);
      }
      else
        this.convValue=this._value;
      this.midilearn=this.getAttr("midilearn",opt.midilearn);
      this.midicc=this.getAttr("midicc",null);

      this.midiController={};
      this.midiMode="normal";
      if(this.midicc) {
          let ch = parseInt(this.midicc.substring(0, this.midicc.lastIndexOf("."))) - 1;
          let cc = parseInt(this.midicc.substring(this.midicc.lastIndexOf(".") + 1));
          this.setMidiController(ch, cc);
      }
      this.setupImage();
      this.digits=0;
      this.coltab=["#e00","#000","#000"];
      if(window.webAudioControlsMidiManager)
        window.webAudioControlsMidiManager.addWidget(this);
    }
    disconnectedCallback(){}
    setupImage(){
      this.kw=this.width||this.diameter;
      this.kh=this.height||this.diameter;
      if(!this.src){
        if(this.colors)
          this.coltab = this.colors.split(";");
        if(!this.coltab)
          this.coltab=["#e00","#000","#000"];
        let svg=
`<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="64" height="6464" preserveAspectRatio="none">
<radialGradient id="gr" cx="30%" cy="30%"><stop offset="0%" stop-color="${this.coltab[2]}"/><stop offset="100%" stop-color="${this.coltab[1]}"/></radialGradient>
<defs><circle id="B" cx="32" cy="32" r="30" fill="url(#gr)"/></defs>
<defs><line id="K" x1="32" y1="28" x2="32" y2="7" stroke-linecap="round" stroke-width="6" stroke="${this.coltab[0]}"/></defs>`;
        for(let i=0;i<101;++i){
          svg += `<use xlink:href="#B" y="${64*i}"/>`;
          svg += `<use xlink:href="#K" y="${64*i}" transform="rotate(${(-135+270*i/101).toFixed(2)},32,${64*i+32})"/>`;
        }
        svg += "</svg>";
        this.elem.style.backgroundImage = "url(data:image/svg+xml;base64,"+btoa(svg)+")";
//        this.elem.style.backgroundSize = "100% 10100%";
        this.elem.style.backgroundSize = `${this.kw}px ${this.kh*101}px`;
      }
      else{
        this.elem.style.backgroundImage = "url("+(this.src)+")";
        if(!this.sprites)
          this.elem.style.backgroundSize = "100% 100%";
        else{
//          this.elem.style.backgroundSize = `100% ${(this.sprites+1)*100}%`;
          this.elem.style.backgroundSize = `${this.kw}px ${this.kh*(this.sprites+1)}px`;
        }
      }
      this.elem.style.outline=this.outline?"":"none";
      this.elem.style.width=this.kw+"px";
      this.elem.style.height=this.kh+"px";
      this.style.height=this.kh+"px";
      this.redraw();
    }
    redraw() {
      this.digits=0;
      if(this.step && this.step < 1) {
        for(let n = this.step ; n < 1; n *= 10)
          ++this.digits;
      }
      if(this.value<this.min){
        this.value=this.min;
        return;
      }
      if(this.value>this.max){
        this.value=this.max;
        return;
      }
      let range = this.max - this.min;
      let style = this.elem.style;
      let sp = this.src?this.sprites:100;
      if(sp>=1){
        let offset = ((sp * (this.value - this.min) / range) | 0);
        style.backgroundPosition = "0px " + (-offset*this.kh) + "px";
        style.transform = 'rotate(0deg)';
      } else {
        let deg = 270 * ((this.value - this.min) / range - 0.5);
        style.backgroundPosition="0px 0px";
        style.transform = 'rotate(' + deg + 'deg)';
      }
    }
    _setValue(v){
      if(this.step)
        v=(Math.round((v-this.min)/this.step))*this.step+this.min;
      this._value=Math.min(this.max,Math.max(this.min,v));
      if(this._value!=this.oldvalue){
        this.oldvalue=this._value;
        if(this.conv){
          const x=this._value;
          this.convValue=eval(this.conv);
          if(typeof(this.convValue)=="function")
            this.convValue=this.convValue(x);
        }
        else
          this.convValue=this._value;
        this.redraw();
        this.showtip(0);
        return 1;
      }
      return 0;
    }
    setValue(v,f){
      if(this._setValue(v) && f)
        this.sendEvent("input"),this.sendEvent("change");
    }
    wheel(e) {
      if (!this.enable)
        return;
      let delta=(this.max-this.min)*0.01;
      delta=e.deltaY>0?-delta:delta;
      if(!e.shiftKey)
        delta*=5;
      if(Math.abs(delta) < this.step)
        delta = (delta > 0) ? +this.step : -this.step;
      this.setValue(+this.value+delta,true);
      e.preventDefault();
      e.stopPropagation();
    }
    pointerdown(ev){
      if(!this.enable)
        return;
      let e=ev;
      if(ev.touches){
        e = ev.changedTouches[0];
        this.identifier=e.identifier;
      }
      else {
        if(e.buttons!=1 && e.button!=0)
          return;
      }
      this.elem.focus();
      this.drag=1;
      this.showtip(0);
      let pointermove=(ev)=>{
        let e=ev;
        if(ev.touches){
          for(let i=0;i<ev.touches.length;++i){
            if(ev.touches[i].identifier==this.identifier){
              e = ev.touches[i];
              break;
            }
          }
        }
        if(this.lastShift !== e.shiftKey) {
          this.lastShift = e.shiftKey;
          this.startPosX = e.pageX;
          this.startPosY = e.pageY;
          this.startVal = this.value;
        }
        let offset = (this.startPosY - e.pageY - this.startPosX + e.pageX) * this.sensitivity;
        this._setValue(this.min + ((((this.startVal + (this.max - this.min) * offset / ((e.shiftKey ? 4 : 1) * 128)) - this.min) / this.step) | 0) * this.step);
        this.sendEvent("input");
        if(e.preventDefault)
          e.preventDefault();
        if(e.stopPropagation)
          e.stopPropagation();
        return false;
      }
      let pointerup=(ev)=>{
        let e=ev;
        if(ev.touches){
          for(let i=0;;){
            if(ev.changedTouches[i].identifier==this.identifier){
              break;
            }
            if(++i>=ev.changedTouches.length)
              return;
          }
        }
        this.drag=0;
        this.showtip(0);
        this.startPosX = this.startPosY = null;
        window.removeEventListener('mousemove', pointermove);
        window.removeEventListener('touchmove', pointermove, {passive:false});
        window.removeEventListener('mouseup', pointerup);
        window.removeEventListener('touchend', pointerup);
        window.removeEventListener('touchcancel', pointerup);
        document.body.removeEventListener('touchstart', preventScroll,{passive:false});
        this.sendEvent("change");
      }
      let preventScroll=(e)=>{
        e.preventDefault();
      }
      if(e.ctrlKey || e.metaKey)
        this.setValue(this.defvalue,true);
      else {
        this.startPosX = e.pageX;
        this.startPosY = e.pageY;
        this.startVal = this.value;
        window.addEventListener('mousemove', pointermove);
        window.addEventListener('touchmove', pointermove, {passive:false});
      }
      window.addEventListener('mouseup', pointerup);
      window.addEventListener('touchend', pointerup);
      window.addEventListener('touchcancel', pointerup);
      document.body.addEventListener('touchstart', preventScroll,{passive:false});
      ev.preventDefault();
      ev.stopPropagation();
      return false;
    }
  });
} catch(error){
  console.log("webaudio-knob already defined");
}

try{
  customElements.define("webaudio-slider", class WebAudioSlider extends WebAudioControlsWidget {
    constructor(){
      super();
    }
    connectedCallback(){
      let root;
//      if(this.attachShadow)
//        root=this.attachShadow({mode: 'open'});
//      else
        root=this;
      root.innerHTML=
`<style>
${this.basestyle}
webaudio-slider{
  display:inline-block;
  position:relative;
  margin:0;
  padding:0;
  font-family: sans-serif;
  font-size: 11px;
  cursor:pointer;
}
.webaudio-slider-body{
  display:inline-block;
  position:relative;
  margin:0;
  padding:0;
  vertical-align:bottom;
}
.webaudio-slider-knob{
  display:inline-block;
  position:absolute;
  margin:0;
  padding:0;
}
</style>
<div class='webaudio-slider-body' tabindex='1' touch-action='none'><div class='webaudio-slider-knob' touch-action='none'></div></div><div class='webaudioctrl-tooltip'></div>
`;
      this.elem=root.childNodes[2];
      this.knob=this.elem.childNodes[0];
      this.ttframe=root.childNodes[3];
      this.enable=this.getAttr("enable",1);
      this.tracking=this.getAttr("tracking","rel"); 
      this._src=this.getAttr("src",opt.sliderSrc); Object.defineProperty(this,"src",{get:()=>{return this._src},set:(v)=>{this._src=v;this.setupImage()}});
      this._knobsrc=this.getAttr("knobsrc",opt.sliderKnobsrc); Object.defineProperty(this,"knobsrc",{get:()=>{return this._knobsrc},set:(v)=>{this._knobsrc=v;this.setupImage()}});
      this._value=this.getAttr("value",0); Object.defineProperty(this,"value",{get:()=>{return this._value},set:(v)=>{this._value=v;this.redraw()}});
      this.defvalue=this.getAttr("defvalue",0);
      this._min=this.getAttr("min",0); Object.defineProperty(this,"min",{get:()=>{return this._min},set:(v)=>{this._min=v;this.redraw()}});
      this._max=this.getAttr("max",100); Object.defineProperty(this,"max",{get:()=>{return this._max},set:(v)=>{this._max=v;this.redraw()}});
      this._step=this.getAttr("step",1); Object.defineProperty(this,"step",{get:()=>{return this._step},set:(v)=>{this._step=v;this.redraw()}});
      this._sprites=this.getAttr("sprites",0); Object.defineProperty(this,"sprites",{get:()=>{return this._sprites},set:(v)=>{this._sprites=v;this.setupImage()}});
      this._direction=this.getAttr("direction",null); Object.defineProperty(this,"direction",{get:()=>{return this._direction},set:(v)=>{this._direction=v;this.setupImage()}});
      this._width=this.getAttr("width",opt.sliderWidth); Object.defineProperty(this,"width",{get:()=>{return this._width},set:(v)=>{this._width=v;this.setupImage()}});
      this._height=this.getAttr("height",opt.sliderHeight); Object.defineProperty(this,"height",{get:()=>{return this._height},set:(v)=>{this._height=v;this.setupImage()}});
      if(this._direction=="horz"){
        if(this._width==0) this._width=128;
        if(this._height==0) this._height=24;
      }
      else{
        if(this._width==0) this._width=24;
        if(this._height==0) this._height=128;
      }
      this._knobwidth=this.getAttr("knobwidth",opt.sliderKnobwidth); Object.defineProperty(this,"knobwidth",{get:()=>{return this._knobwidth},set:(v)=>{this._knobwidth=v;this.setupImage()}});
      this._knobheight=this.getAttr("knbheight",opt.sliderKnobheight); Object.defineProperty(this,"knobheight",{get:()=>{return this._knobheight},set:(v)=>{this._knobheight=v;this.setupImage()}});
      this._ditchlength=this.getAttr("ditchlength",opt.sliderDitchlength); Object.defineProperty(this,"ditchlength",{get:()=>{return this._ditchlength},set:(v)=>{this._ditchlength=v;this.setupImage()}});
      this._colors=this.getAttr("colors",opt.sliderColors); Object.defineProperty(this,"colors",{get:()=>{return this._colors},set:(v)=>{this._colors=v;this.setupImage()}});
      this.outline=this.getAttr("outline",opt.outline);
      this.sensitivity=this.getAttr("sensitivity",1);
      this.valuetip=this.getAttr("valuetip",opt.valuetip);
      this.tooltip=this.getAttr("tooltip",null);
      this.conv=this.getAttr("conv",null);
      if(this.conv){
        const x=this._value;
        this.convValue=eval(this.conv);
        if(typeof(this.convValue)=="function")
          this.convValue=this.convValue(x);
      }
      else
        this.convValue=this._value;
      this.midilearn=this.getAttr("midilearn",opt.midilearn);
      this.midicc=this.getAttr("midicc",null);
      this.midiController={};
      this.midiMode="normal";
      if(this.midicc) {
          let ch = parseInt(this.midicc.substring(0, this.midicc.lastIndexOf("."))) - 1;
          let cc = parseInt(this.midicc.substring(this.midicc.lastIndexOf(".") + 1));
          this.setMidiController(ch, cc);
      }
      this.setupImage();
      this.digits=0;
      if(window.webAudioControlsMidiManager)
//        window.webAudioControlsMidiManager.updateWidgets();
        window.webAudioControlsMidiManager.addWidget(this);
      this.elem.onclick=(e)=>{e.stopPropagation()};
    }
    disconnectedCallback(){}
    setupImage(){
      this.coltab = this.colors.split(";");
      this.dr=this.direction;
      this.dlen=this.ditchlength;
      if(!this.width){
        if(this.dr=="horz")
          this.width=128;
        else
          this.width=24;
      }
      if(!this.height){
        if(this.dr=="horz")
          this.height=24;
        else
          this.height=128;
      }
      if(!this.dr)
        this.dr=(this.width<=this.height)?"vert":"horz";
      if(this.dr=="vert"){
        if(!this.dlen)
          this.dlen=this.height-this.width;
      }
      else{
        if(!this.dlen)
          this.dlen=this.width-this.height;
      }
      this.knob.style.backgroundSize = "100% 100%";
      this.elem.style.backgroundSize = "100% 100%";
      this.elem.style.width=this.width+"px";
      this.elem.style.height=this.height+"px";
      this.style.height=this.height+"px";
      this.kwidth=this.knobwidth||(this.dr=="horz"?this.height:this.width);
      this.kheight=this.knobheight||(this.dr=="horz"?this.height:this.width);
      this.knob.style.width = this.kwidth+"px";
      this.knob.style.height = this.kheight+"px";
      if(!this.src){
        let r=Math.min(this.width,this.height)*0.5;
        let svgbody=
`<svg xmlns="http://www.w3.org/2000/svg" width="${this.width}" height="${this.height}" preserveAspectRatio="none">
<rect x="1" y="1" rx="${r}" ry="${r}" width="${this.width-2}" height="${this.height-2}" fill="${this.coltab[1]}"/></svg>`;
        this.elem.style.backgroundImage = "url(data:image/svg+xml;base64,"+btoa(svgbody)+")";
      }
      else{
        this.elem.style.backgroundImage = "url("+(this.src)+")";
      }
      if(!this.knobsrc){
        let svgthumb=
`<svg xmlns="http://www.w3.org/2000/svg" width="${this.kwidth}" height="${this.kheight}" preserveAspectRatio="none">
<radialGradient id="gr" cx="30%" cy="30%"><stop offset="0%" stop-color="${this.coltab[2]}"/><stop offset="100%" stop-color="${this.coltab[0]}"/></radialGradient>
<rect x="2" y="2" width="${this.kwidth-4}" height="${this.kheight-4}" rx="${this.kwidth*0.5}" ry="${this.kheight*0.5}" fill="url(#gr)"/></svg>`;
        this.knob.style.backgroundImage = "url(data:image/svg+xml;base64,"+btoa(svgthumb)+")";
      }
      else{
        this.knob.style.backgroundImage = "url("+(this.knobsrc)+")";
      }
      this.elem.style.outline=this.outline?"":"none";
      this.redraw();
    }
    redraw() {
      this.digits=0;
      if(this.step && this.step < 1) {
        for(let n = this.step ; n < 1; n *= 10)
          ++this.digits;
      }
      if(this.value<this.min){
        this.value=this.min;
        return;
      }
      if(this.value>this.max){
        this.value=this.max;
        return;
      }
      let range = this.max - this.min;
      let style = this.knob.style;
      if(this.dr=="vert"){
        style.left=(this.width-this.kwidth)*0.5+"px";
        style.top=(1-(this.value-this.min)/range)*this.dlen+"px";
        this.sensex=0; this.sensey=1;
      }
      else{
        style.top=(this.height-this.kheight)*0.5+"px";
        style.left=(this.value-this.min)/range*this.dlen+"px";
        this.sensex=1; this.sensey=0;
      }
    }
    _setValue(v){
      v=(Math.round((v-this.min)/this.step))*this.step+this.min;
      this._value=Math.min(this.max,Math.max(this.min,v));
      if(this._value!=this.oldvalue){
        this.oldvalue=this._value;
        if(this.conv){
          const x=this._value;
          this.convValue=eval(this.conv);
          if(typeof(this.convValue)=="function")
            this.convValue=this.convValue(x);
        }
        else
          this.convValue=this._value;
        this.redraw();
        this.showtip(0);
        return 1;
      }
      return 0;
    }
    setValue(v,f){
      if(this._setValue(v)&&f)
        this.sendEvent("input"),this.sendEvent("change");
    }
    wheel(e) {
      let delta=(this.max-this.min)*0.01;
      delta=e.deltaY>0?-delta:delta;
      if(!e.shiftKey)
        delta*=5;
      if(Math.abs(delta) < this.step)
        delta = (delta > 0) ? +this.step : -this.step;
      this.setValue(+this.value+delta,true);
      e.preventDefault();
      e.stopPropagation();
      this.redraw();
    }
    pointerdown(ev){
      if(!this.enable)
        return;
      let e=ev;
      if(ev.touches){
        e = ev.changedTouches[0];
        this.identifier=e.identifier;
      }
      else {
        if(e.buttons!=1 && e.button!=0)
          return;
      }
      this.elem.focus();
      this.drag=1;
      this.showtip(0);
      let pointermove=(ev)=>{
        let e=ev;
        if(ev.touches){
          for(let i=0;i<ev.touches.length;++i){
            if(ev.touches[i].identifier==this.identifier){
              e = ev.touches[i];
              break;
            }
          }
        }
        if(this.lastShift !== e.shiftKey) {
          this.lastShift = e.shiftKey;
          this.startPosX = e.pageX;
          this.startPosY = e.pageY;
          this.startVal = this.value;
        }
        if(this.tracking=="abs"){
          const rc = this.getBoundingClientRect();
          let val;
          if(this.direction=="horz")
            val = Math.max(0,Math.min(1,(e.pageX-rc.left-this.kwidth*0.5)/(this.width-this.kwidth)));
          else
            val = 1 - Math.max(0,Math.min(1,(e.pageY-rc.top-this.kheight*0.5)/(this.height-this.kheight)));
          this._setValue(this.min + (this.max - this.min)*val);
        }
        else{
          let offset = ((this.startPosY - e.pageY)*this.sensey - (this.startPosX - e.pageX)*this.sensex) * this.sensitivity;
          this._setValue(this.min + ((((this.startVal + (this.max - this.min) * offset / ((e.shiftKey ? 4 : 1) * this.dlen)) - this.min) / this.step) | 0) * this.step);
        }
        this.sendEvent("input");
        if(e.preventDefault)
          e.preventDefault();
        if(e.stopPropagation)
          e.stopPropagation();
        return false;
      }
      let pointerup=(ev)=>{
        let e=ev;
        if(ev.touches){
          for(let i=0;;){
            if(ev.changedTouches[i].identifier==this.identifier){
              break;
            }
            if(++i>=ev.changedTouches.length)
              return;
          }
        }
        this.drag=0;
        this.showtip(0);
        this.startPosX = this.startPosY = null;
        window.removeEventListener('mousemove', pointermove);
        window.removeEventListener('touchmove', pointermove, {passive:false});
        window.removeEventListener('mouseup', pointerup);
        window.removeEventListener('touchend', pointerup);
        window.removeEventListener('touchcancel', pointerup);
        document.body.removeEventListener('touchstart', preventScroll,{passive:false});
        this.sendEvent("change");
      }
      let preventScroll=(e)=>{
        e.preventDefault();
      }
      if(e.touches)
        e = e.touches[0];
      if(e.ctrlKey || e.metaKey)
        this.setValue(this.defvalue,true);
      else {
        this.startPosX = e.pageX;
        this.startPosY = e.pageY;
        this.startVal = this.value;
        window.addEventListener('mousemove', pointermove);
        window.addEventListener('touchmove', pointermove, {passive:false});
      }
      window.addEventListener('mouseup', pointerup);
      window.addEventListener('touchend', pointerup);
      window.addEventListener('touchcancel', pointerup);
      document.body.addEventListener('touchstart', preventScroll,{passive:false});
      pointermove(ev);
      e.preventDefault();
      e.stopPropagation();
      return false;
    }
  });
} catch(error){
  console.log("webaudio-slider already defined");
}

try{
  customElements.define("webaudio-switch", class WebAudioSwitch extends WebAudioControlsWidget {
    constructor(){
      super();
    }
    connectedCallback(){
      let root;
//      if(this.attachShadow)
//        root=this.attachShadow({mode: 'open'});
//      else
        root=this;
      root.innerHTML=
`<style>
${this.basestyle}
webaudio-switch{
  display:inline-block;
  margin:0;
  padding:0;
  font-family: sans-serif;
  font-size: 11px;
  cursor:pointer;
}
.webaudio-switch-body{
  display:inline-block;
  margin:0;
  padding:0;
  vertical-align:bottom;
}
</style>
<div class='webaudio-switch-body' tabindex='1' touch-action='none'><div class='webaudioctrl-tooltip'></div></div>
`;
      this.elem=root.childNodes[2];
      this.ttframe=this.elem.childNodes[0];

      this.enable=this.getAttr("enable",1);
      this._src=this.getAttr("src",null); Object.defineProperty(this,"src",{get:()=>{return this._src},set:(v)=>{this._src=v;this.setupImage()}});
      this._value=this.getAttr("value",0); Object.defineProperty(this,"value",{get:()=>{return this._value},set:(v)=>{this._value=v;this.redraw()}});
      this.defvalue=this.getAttr("defvalue",0);
      this.type=this.getAttr("type","toggle");
      this.group=this.getAttr("group","");
      this._width=this.getAttr("width",0); Object.defineProperty(this,"width",{get:()=>{return this._width},set:(v)=>{this._width=v;this.setupImage()}});
      this._height=this.getAttr("height",0); Object.defineProperty(this,"height",{get:()=>{return this._height},set:(v)=>{this._height=v;this.setupImage()}});
      this._diameter=this.getAttr("diameter",0); Object.defineProperty(this,"diameter",{get:()=>{return this._diameter},set:(v)=>{this._diameter=v;this.setupImage()}});
      this.invert=this.getAttr("invert",0);
      this._colors=this.getAttr("colors",opt.switchColors); Object.defineProperty(this,"colors",{get:()=>{return this._colors},set:(v)=>{this._colors=v;this.setupImage()}});
      this.outline=this.getAttr("outline",opt.outline);
      this.valuetip=0;
      this.tooltip=this.getAttr("tooltip",null);
      this.midilearn=this.getAttr("midilearn",opt.midilearn);
      this.midicc=this.getAttr("midicc",null);
      this.midiController={};
      this.midiMode="normal";
      if(this.midicc) {
          let ch = parseInt(this.midicc.substring(0, this.midicc.lastIndexOf("."))) - 1;
          let cc = parseInt(this.midicc.substring(this.midicc.lastIndexOf(".") + 1));
          this.setMidiController(ch, cc);
      }
      this.setupImage();
      this.digits=0;
      if(window.webAudioControlsMidiManager)
//        window.webAudioControlsMidiManager.updateWidgets();
        window.webAudioControlsMidiManager.addWidget(this);
      this.elem.onclick=(e)=>{e.stopPropagation()};
    }
    disconnectedCallback(){}
    setupImage(){
      let w=this.width||this.diameter||opt.switchWidth||opt.switchDiameter;
      let h=this.height||this.diameter||opt.switchHeight||opt.switchDiameter;
      if(!this.src){
        this.coltab = this.colors.split(";");
        let mm=Math.min(w,h);
        let svg=
`<svg xmlns="http://www.w3.org/2000/svg" width="${w}" height="${h*2}" preserveAspectRatio="none">
<radialGradient id="gr" cx="30%" cy="30%"><stop offset="0%" stop-color="${this.coltab[2]}"/><stop offset="100%" stop-color="${this.coltab[0]}"/></radialGradient>
<rect x="${w*0.05}" y="${h*0.05}" width="${w*0.9}" height="${h*0.9}" rx="${mm*0.1}" ry="${mm*0.1}" fill="${this.coltab[1]}"/>
<rect x="${w*0.05}" y="${h*1.05}" width="${w*0.9}" height="${h*0.9}" rx="${mm*0.1}" ry="${mm*0.1}" fill="${this.coltab[1]}"/>
<circle cx="${w*0.5}" cy="${h*0.5}" r="${mm*0.3}" stroke="${this.coltab[0]}" stroke-width="2"/>
<circle cx="${w*0.5}" cy="${h*1.5}" r="${mm*0.3}" stroke="${this.coltab[0]}" stroke-width="2" fill="url(#gr)"/></svg>`;
        this.elem.style.backgroundImage = "url(data:image/svg+xml;base64,"+btoa(svg)+")";
        this.elem.style.backgroundSize = "100% 200%";
      }
      else{
        this.elem.style.backgroundImage = "url("+(this.src)+")";
        if(!this.sprites)
          this.elem.style.backgroundSize = "100% 200%";
        else
          this.elem.style.backgroundSize = `100% ${(this.sprites+1)*100}%`;
      }
      this.elem.style.width=w+"px";
      this.elem.style.height=h+"px";
      this.style.height=h+"px";
      this.elem.style.outline=this.outline?"":"none";
      this.redraw();
    }
    redraw() {
      let style = this.elem.style;
      if(this.value^this.invert)
        style.backgroundPosition = "0px -100%";
      else
        style.backgroundPosition = "0px 0px";
    }
    setValue(v,f){
      this.value=v;
      this.checked=(!!v);
      if(this.value!=this.oldvalue){
        this.redraw();
        this.showtip(0);
        if(f){
          this.sendEvent("input");
          this.sendEvent("change");
        }
        this.oldvalue=this.value;
      }
    }
    pointerdown(ev){
      if(!this.enable)
        return;
      let e=ev;
      if(ev.touches){
        e = ev.changedTouches[0];
        this.identifier=e.identifier;
      }
      else {
        if(e.buttons!=1 && e.button!=0)
          return;
      }
      this.elem.focus();
      this.drag=1;
      this.showtip(0);
      let pointermove=(e)=>{
        e.preventDefault();
        e.stopPropagation();
        return false;
      }
      let pointerup=(e)=>{
        this.drag=0;
        this.showtip(0);
        window.removeEventListener('mousemove', pointermove);
        window.removeEventListener('touchmove', pointermove, {passive:false});
        window.removeEventListener('mouseup', pointerup);
        window.removeEventListener('touchend', pointerup);
        window.removeEventListener('touchcancel', pointerup);
        document.body.removeEventListener('touchstart', preventScroll,{passive:false});
        if(this.type=="kick"){
          this.value=0;
          this.checked=false;
          this.redraw();
          this.sendEvent("change");
        }
        this.sendEvent("click");
        e.preventDefault();
        e.stopPropagation();
      }
      let preventScroll=(e)=>{
        e.preventDefault();
      }
      switch(this.type){
      case "kick":
        this.setValue(1);
        this.sendEvent("change");
        break;
      case "toggle":
        if(e.ctrlKey || e.metaKey)
          this.value=defvalue;
        else
          this.value=1-this.value;
        this.checked=!!this.value;
        this.sendEvent("change");
        break;
      case "radio":
        let els=document.querySelectorAll("webaudio-switch[type='radio'][group='"+this.group+"']");
        for(let i=0;i<els.length;++i){
          if(els[i]==this)
            els[i].setValue(1);
          else
            els[i].setValue(0);
        }
        this.sendEvent("change");
        break;
      }

      window.addEventListener('mouseup', pointerup);
      window.addEventListener('touchend', pointerup);
      window.addEventListener('touchcancel', pointerup);
      document.body.addEventListener('touchstart', preventScroll,{passive:false});
      this.redraw();
      e.preventDefault();
      e.stopPropagation();
      return false;
    }
  });
} catch(error){
  console.log("webaudio-switch already defined");
}

try{
  customElements.define("webaudio-param", class WebAudioParam extends WebAudioControlsWidget {
    constructor(){
      super();
      this.addEventListener("keydown",this.keydown);
      this.addEventListener("mousedown",this.pointerdown,{passive:false});
      this.addEventListener("touchstart",this.pointerdown,{passive:false});
      this.addEventListener("wheel",this.wheel);
      this.addEventListener("mouseover",this.pointerover);
      this.addEventListener("mouseout",this.pointerout);
      this.addEventListener("contextmenu",this.contextMenu);
    }
    connectedCallback(){
      let root;
//      if(this.attachShadow)
//        root=this.attachShadow({mode: 'open'});
//      else
        root=this;
      root.innerHTML=
`<style>
${this.basestyle}
webaudio-param{
  display:inline-block;
  user-select:none;
  margin:0;
  padding:0;
  font-family: sans-serif;
  font-size: 8px;
  cursor:pointer;
  position:relative;
  vertical-align:baseline;
}
.webaudio-param-body{
  display:inline-block;
  position:relative;
  text-align:center;
  border:1px solid #888;
  background:none;
  border-radius:4px;
  margin:0;
  padding:0;
  font-family:sans-serif;
  font-size:11px;
  vertical-align:bottom;
}
</style>
<input class='webaudio-param-body' value='0' tabindex='1' touch-action='none'/><div class='webaudioctrl-tooltip'></div>
`;
      this.elem=root.childNodes[2];
      this.ttframe=root.childNodes[3];
      this.enable=this.getAttr("enable",1);
      this._value=this.getAttr("value",0); Object.defineProperty(this,"value",{get:()=>{return this._value},set:(v)=>{this._value=v;this.redraw()}});
      this.defvalue=this.getAttr("defvalue",0);
      this._fontsize=this.getAttr("fontsize",9); Object.defineProperty(this,"fontsize",{get:()=>{return this._fontsize},set:(v)=>{this._fontsize=v;this.setupImage()}});
      this._src=this.getAttr("src",null); Object.defineProperty(this,"src",{get:()=>{return this._src},set:(v)=>{this._src=v;this.setupImage()}});
      this.link=this.getAttr("link","");
      this._width=this.getAttr("width",32); Object.defineProperty(this,"width",{get:()=>{return this._width},set:(v)=>{this._width=v;this.setupImage()}});
      this._height=this.getAttr("height",20); Object.defineProperty(this,"height",{get:()=>{return this._height},set:(v)=>{this._height=v;this.setupImage()}});
      this._colors=this.getAttr("colors","#fff;#000"); Object.defineProperty(this,"colors",{get:()=>{return this._colors},set:(v)=>{this._colors=v;this.setupImage()}});
      this.outline=this.getAttr("outline",opt.outline);
      this.rconv=this.getAttr("rconv",null);
      this.midiController={};
      this.midiMode="normal";
      this.currentLink=null;
      if(this.midicc) {
        let ch = parseInt(this.midicc.substring(0, this.midicc.lastIndexOf("."))) - 1;
        let cc = parseInt(this.midicc.substring(this.midicc.lastIndexOf(".") + 1));
        this.setMidiController(ch, cc);
      }
      this.setupImage();
      if(window.webAudioControlsMidiManager)
//        window.webAudioControlsMidiManager.updateWidgets();
        window.webAudioControlsMidiManager.addWidget(this);
      this.fromLink=((e)=>{
        this.setValue(e.target.convValue.toFixed(e.target.digits));
      }).bind(this);
      this.elem.onchange=()=>{
        if(!this.currentLink.target.conv || (this.currentLink.target.conv&&this.rconv)){
          let val = this.value=this.elem.value;
          if(this.rconv){
            let x=+this.elem.value;
            val=eval(this.rconv);
          }
          if(this.currentLink){
            this.currentLink.target.setValue(val);
          }
        }
      }
    }
    disconnectedCallback(){}
    setupImage(){
      this.coltab = this.colors.split(";");
      this.elem.style.color=this.coltab[0];
      if(!this.src){
        this.elem.style.backgroundColor=this.coltab[1];
      }
      else{
        this.elem.style.backgroundImage = "url("+(this.src)+")";
        this.elem.style.backgroundSize = "100% 100%";
      }
      this.elem.style.width=this.width+"px";
      this.elem.style.height=this.height+"px";
      this.elem.style.fontSize=this.fontsize+"px";
      this.elem.style.outline=this.outline?"":"none";
      let l=document.getElementById(this.link);
      if(l&&typeof(l.value)!="undefined"){
        this.setValue(l.convValue.toFixed(l.digits));
        if(this.currentLink)
          this.currentLink.removeEventListener("input",this.currentLink.func);
        this.currentLink={target:l, func:(e)=>{this.setValue(l.convValue.toFixed(l.digits))}};
        this.currentLink.target.addEventListener("input",this.currentLink.func);
//        l.addEventListener("input",(e)=>{this.setValue(l.convValue.toFixed(l.digits))});
      }
      this.redraw();
    }
    redraw() {
      this.elem.value=this.value;
    }
    setValue(v,f){
      this.value=v;
      if(this.value!=this.oldvalue){
        this.redraw();
        this.showtip(0);
        if(f){
          let event=document.createEvent("HTMLEvents");
          event.initEvent("change",false,true);
          this.dispatchEvent(event);
        }
        this.oldvalue=this.value;
      }
    }
    pointerdown(ev){
      if(!this.enable)
        return;
      let e=ev;
      if(ev.touches)
          e = ev.touches[0];
      else {
        if(e.buttons!=1 && e.button!=0)
          return;
      }
      this.elem.focus();
      this.redraw();
    }
  });
} catch(error){
  console.log("webaudio-param already defined");
}

try{
  customElements.define("webaudio-keyboard", class WebAudioKeyboard extends WebAudioControlsWidget {
    constructor(){
      super();
    }
    connectedCallback(){
      let root;
//      if(this.attachShadow)
//        root=this.attachShadow({mode: 'open'});
//      else
        root=this;
      root.innerHTML=
`<style>
${this.basestyle}
webaudio-keyboard{
  display:inline-block;
  position:relative;
  margin:0;
  padding:0;
  font-family: sans-serif;
  font-size: 11px;
}
.webaudio-keyboard-body{
  display:inline-block;
  margin:0;
  padding:0;
  vertical-align:bottom;
}
</style>
<canvas class='webaudio-keyboard-body' tabindex='1' touch-action='none'></canvas><div class='webauioctrl-tooltip'></div>
`;
      this.cv=root.childNodes[2];
      this.ttframe=root.childNodes[3];
      this.ctx=this.cv.getContext("2d");
      this._values=[];
      this.enable=this.getAttr("enable",1);
      this._width=this.getAttr("width",480); Object.defineProperty(this,"width",{get:()=>{return this._width},set:(v)=>{this._width=v;this.setupImage()}});
      this._height=this.getAttr("height",128); Object.defineProperty(this,"height",{get:()=>{return this._height},set:(v)=>{this._height=v;this.setupImage()}});
      this._min=this.getAttr("min",0); Object.defineProperty(this,"min",{get:()=>{return this._min},set:(v)=>{this._min=+v;this.redraw()}});
      this._keys=this.getAttr("keys",25); Object.defineProperty(this,"keys",{get:()=>{return this._keys},set:(v)=>{this._keys=+v;this.setupImage()}});
      this._colors=this.getAttr("colors","#222;#eee;#ccc;#333;#000;#e88;#c44;#c33;#800"); Object.defineProperty(this,"colors",{get:()=>{return this._colors},set:(v)=>{this._colors=v;this.setupImage()}});
      this.outline=this.getAttr("outline",opt.outline);
      this.midilearn=this.getAttr("midilearn",0);
      this.midicc=this.getAttr("midicc",null);
      this.press=0;
      this.keycodes1=[90,83,88,68,67,86,71,66,72,78,74,77,188,76,190,187,191,226];
      this.keycodes2=[81,50,87,51,69,82,53,84,54,89,55,85,73,57,79,48,80,192,222,219];
      this.addEventListener("keyup",this.keyup);
      this.midiController={};
      this.midiMode="normal";
      if(this.midicc) {
          let ch = parseInt(this.midicc.substring(0, this.midicc.lastIndexOf("."))) - 1;
          let cc = parseInt(this.midicc.substring(this.midicc.lastIndexOf(".") + 1));
          this.setMidiController(ch, cc);
      }
      this.setupImage();
      this.digits=0;
      if(window.webAudioControlsMidiManager)
        window.webAudioControlsMidiManager.addWidget(this);
    }
    disconnectedCallback(){}
    setupImage(){
      this.cv.style.width=this.width+"px";
      this.cv.style.height=this.height+"px";
      this.bheight = this.height * 0.55;
      this.kp=[0,7/12,1,3*7/12,2,3,6*7/12,4,8*7/12,5,10*7/12,6];
      this.kf=[0,1,0,1,0,0,1,0,1,0,1,0];
      this.ko=[0,0,(7*2)/12-1,0,(7*4)/12-2,(7*5)/12-3,0,(7*7)/12-4,0,(7*9)/12-5,0,(7*11)/12-6];
      this.kn=[0,2,4,5,7,9,11];
      this.coltab=this.colors.split(";");
      this.cv.width = this.width;
      this.cv.height = this.height;
      this.cv.style.width = this.width+'px';
      this.cv.style.height = this.height+'px';
      this.style.height = this.height+'px';
      this.cv.style.outline=this.outline?"":"none";
      this.bheight = this.height * 0.55;
      this.max=this.min+this.keys-1;
      this.dispvalues=[];
      this.valuesold=[];
      if(this.kf[this.min%12])
        --this.min;
      if(this.kf[this.max%12])
        ++this.max;
      this.redraw();
    }
    redraw(){
      function rrect(ctx, x, y, w, h, r, c1, c2) {
        if(c2) {
          let g=ctx.createLinearGradient(x,y,x+w,y);
          g.addColorStop(0,c1);
          g.addColorStop(1,c2);
          ctx.fillStyle=g;
        }
        else
          ctx.fillStyle=c1;
        ctx.beginPath();
        ctx.moveTo(x, y);
        ctx.lineTo(x+w, y);
        ctx.lineTo(x+w, y+h-r);
        ctx.quadraticCurveTo(x+w, y+h, x+w-r, y+h);
        ctx.lineTo(x+r, y+h);
        ctx.quadraticCurveTo(x, y+h, x, y+h-r);
        ctx.lineTo(x, y);
        ctx.fill();
      }
      this.ctx.fillStyle = this.coltab[0];
      this.ctx.fillRect(0,0,this.width,this.height);
      let x0=7*((this.min/12)|0)+this.kp[this.min%12];
      let x1=7*((this.max/12)|0)+this.kp[this.max%12];
      let n=x1-x0;
      this.wwidth=(this.width-1)/(n+1);
      this.bwidth=this.wwidth*7/12;
      let h2=this.bheight;
      let r=Math.min(8,this.wwidth*0.2);
      for(let i=this.min,j=0;i<=this.max;++i) {
        if(this.kf[i%12]==0) {
          let x=this.wwidth*(j++)+1;
          if(this.dispvalues.indexOf(i)>=0)
            rrect(this.ctx,x,1,this.wwidth-1,this.height-2,r,this.coltab[5],this.coltab[6]);
          else
            rrect(this.ctx,x,1,this.wwidth-1,this.height-2,r,this.coltab[1],this.coltab[2]);
        }
      }
      r=Math.min(8,this.bwidth*0.3);
      for(let i=this.min;i<this.max;++i) {
        if(this.kf[i%12]) {
          let x=this.wwidth*this.ko[this.min%12]+this.bwidth*(i-this.min)+1;
          if(this.dispvalues.indexOf(i)>=0)
            rrect(this.ctx,x,1,this.bwidth,h2,r,this.coltab[7],this.coltab[8]);
          else
            rrect(this.ctx,x,1,this.bwidth,h2,r,this.coltab[3],this.coltab[4]);
          this.ctx.strokeStyle=this.coltab[0];
          this.ctx.stroke();
        }
      }
    }
    _setValue(v){
      if(this.step)
        v=(Math.round((v-this.min)/this.step))*this.step+this.min;
      this._value=Math.min(this.max,Math.max(this.min,v));
      if(this._value!=this.oldvalue){
        this.oldvalue=this._value;
        this.redraw();
        this.showtip(0);
        return 1;
      }
      return 0;
    }
    setValue(v,f){
      if(this._setValue(v) && f)
        this.sendEvent("input"),this.sendEvent("change");
    }
    wheel(e){}
    keydown(e){
      let m=Math.floor((this.min+11)/12)*12;
      let k=this.keycodes1.indexOf(e.keyCode);
      if(k<0) {
        k=this.keycodes2.indexOf(e.keyCode);
        if(k>=0) k+=12;
      }
      if(k>=0){
        k+=m;
        if(this.currentKey!=k){
          this.currentKey=k;
          this.sendEventFromKey(1,k);
          this.setNote(1,k);
        }
      }
    }
    keyup(e){
      let m=Math.floor((this.min+11)/12)*12;
      let k=this.keycodes1.indexOf(e.keyCode);
      if(k<0) {
        k=this.keycodes2.indexOf(e.keyCode);
        if(k>=0) k+=12;
      }
      if(k>=0){
        k+=m;
        this.currentKey=-1;
        this.sendEventFromKey(0,k);
        this.setNote(0,k);
      }
    }
    pointerdown(ev){
      this.cv.focus();
      if(this.enable) {
        ++this.press;
      }
      let pointermove=(ev)=>{
        if(!this.enable)
          return;
        let r=this.getBoundingClientRect();
        let v=[],p;
        if(ev.touches)
          p=ev.targetTouches;
        else if(this.press)
          p=[ev];
        else
          p=[];
        if(p.length>0)
          this.drag=1;
        for(let i=0;i<p.length;++i) {
          let px=p[i].clientX-r.left;
          let py=p[i].clientY-r.top;
          let x,k,ko;
          if(py>=0&&py<this.height){
            if(py<this.bheight) {
              x=px-this.wwidth*this.ko[this.min%12];
              k=this.min+((x/this.bwidth)|0);
            }
            else {
              k=(px/this.wwidth)|0;
              ko=this.kp[this.min%12];
              k+=ko;
              k=this.min+((k/7)|0)*12+this.kn[k%7]-this.kn[ko%7];
            }
            if(k>=this.min&&k<=this.max)
              v.push(k);
          }
        }
        v.sort();
        this.values=v;
        this.sendevent();
        this.redraw();
      }
        
      let pointerup=(ev)=>{
        if(this.enable) {
          if(ev.touches)
            this.press=ev.touches.length;
          else
            this.press=0;
          pointermove(ev);
          this.sendevent();
          if(this.press==0){
            window.removeEventListener('mousemove', pointermove);
            window.removeEventListener('touchmove', pointermove, {passive:false});
            window.removeEventListener('mouseup', pointerup);
            window.removeEventListener('touchend', pointerup);
            window.removeEventListener('touchcancel', pointerup);
            document.body.removeEventListener('touchstart', preventScroll,{passive:false});
          }
          this.redraw();
        }
        this.drag=0;
        ev.preventDefault();
      }
      let preventScroll=(ev)=>{
        ev.preventDefault();
      }
      window.addEventListener('mousemove', pointermove);
      window.addEventListener('touchmove', pointermove, {passive:false});
      window.addEventListener('mouseup', pointerup);
      window.addEventListener('touchend', pointerup);
      window.addEventListener('touchcancel', pointerup);
      document.body.addEventListener('touchstart', preventScroll,{passive:false});
      pointermove(ev);
      ev.preventDefault();
      ev.stopPropagation();
    }
    sendEventFromKey(s,k){
      let ev=document.createEvent('HTMLEvents');
      ev.initEvent('change',true,true);
      ev.note=[s,k];
      this.dispatchEvent(ev);
    }
    sendevent(){
      let notes=[];
      for(let i=0,j=this.valuesold.length;i<j;++i) {
        if(this.values.indexOf(this.valuesold[i])<0)
          notes.push([0,this.valuesold[i]]);
      }
      for(let i=0,j=this.values.length;i<j;++i) {
        if(this.valuesold.indexOf(this.values[i])<0)
          notes.push([1,this.values[i]]);
      }
      if(notes.length) {
        this.valuesold=this.values;
        for(let i=0;i<notes.length;++i) {
          this.setdispvalues(notes[i][0],notes[i][1]);
          let ev=document.createEvent('HTMLEvents');
          ev.initEvent('change',true,true);
          ev.note=notes[i];
          this.dispatchEvent(ev);
        }
      }
    }
    setdispvalues(state,note) {
      let n=this.dispvalues.indexOf(note);
      if(state) {
        if(n<0) this.dispvalues.push(note);
      }
      else {
        if(n>=0) this.dispvalues.splice(n,1);
      }
    }
    setNote(state,note) {
      this.setdispvalues(state,note);
      this.redraw();
    }
  });
} catch(error){
  console.log("webaudio-keyboard already defined");
}

try{
  customElements.define("webaudio-xypad", class WebAudioXYPad extends WebAudioControlsWidget {
    constructor(){
      super();
    }
    connectedCallback(){
      let root;
//      if(this.attachShadow)
//        root=this.attachShadow({mode: 'open'});
//      else
        root=this;
      root.innerHTML=
`<style>
${this.basestyle}
webaudio-xypad{
  display:inline-block;
  position:relative;
  margin:0;
  padding:0;
  font-family: sans-serif;
  font-size: 11px;
  cursor:pointer;
}
.webaudio-xypad-body{
  display:inline-block;
  position:relative;
  margin:0;
  padding:0;
  vertical-align:bottom;
}
.webaudio-xypad-knob{
  display:inline-block;
  position:absolute;
  margin:0;
  padding:0;
}
</style>
<div class='webaudio-xypad-body' tabindex='1' touch-action='none'><div class='webaudio-xypad-knob' touch-action='none'></div></div><div class='webaudioctrl-tooltip'></div>
`;
      this.elem=root.childNodes[2];
      this.knob=this.elem.childNodes[0];
      this.ttframe=root.childNodes[3];

      this.enable=this.getAttr("enable",1);
      this._src=this.getAttr("src",opt.sliderSrc); Object.defineProperty(this,"src",{get:()=>{return this._src},set:(v)=>{this._src=v;this.setupImage()}});
      this._knobsrc=this.getAttr("knobsrc",opt.sliderKnobsrc); Object.defineProperty(this,"knobsrc",{get:()=>{return this._knobsrc},set:(v)=>{this._knobsrc=v;this.setupImage()}});
      this._x=this.getAttr("x",50); Object.defineProperty(this,"x",{get:()=>{return this._x},set:(v)=>{this._x=v;this.redraw()}});
      this._y=this.getAttr("y",50); Object.defineProperty(this,"y",{get:()=>{return this._y},set:(v)=>{this._y=v;this.redraw()}});
      this.defx=this.getAttr("defx",50);
      this.defy=this.getAttr("defy",50);
      this._min=this.getAttr("min",0); Object.defineProperty(this,"min",{get:()=>{return this._min},set:(v)=>{this._min=v;this.redraw()}});
      this._max=this.getAttr("max",100); Object.defineProperty(this,"max",{get:()=>{return this._max},set:(v)=>{this._max=v;this.redraw()}});
      this._step=this.getAttr("step",1); Object.defineProperty(this,"step",{get:()=>{return this._step},set:(v)=>{this._step=v;this.redraw()}});
      this._sprites=this.getAttr("sprites",0); Object.defineProperty(this,"sprites",{get:()=>{return this._sprites},set:(v)=>{this._sprites=v;this.setupImage()}});
      this._width=this.getAttr("width",128); Object.defineProperty(this,"width",{get:()=>{return this._width},set:(v)=>{this._width=v;this.setupImage()}});
      this._height=this.getAttr("height",128); Object.defineProperty(this,"height",{get:()=>{return this._height},set:(v)=>{this._height=v;this.setupImage()}});
      this._knobwidth=this.getAttr("knobwidth",28); Object.defineProperty(this,"knobwidth",{get:()=>{return this._knobwidth},set:(v)=>{this._knobwidth=v;this.setupImage()}});
      this._knobheight=this.getAttr("knbheight",28); Object.defineProperty(this,"knobheight",{get:()=>{return this._knobheight},set:(v)=>{this._knobheight=v;this.setupImage()}});
      this._colors=this.getAttr("colors",opt.sliderColors); Object.defineProperty(this,"colors",{get:()=>{return this._colors},set:(v)=>{this._colors=v;this.setupImage()}});
      this.outline=this.getAttr("outline",opt.outline);
      this.valuetip=this.getAttr("valuetip",opt.valuetip);
      this.tooltip=this.getAttr("tooltip",null);
      this.conv=this.getAttr("conv",null);
      if(this.conv){
        const x=this._x;
        const y=this._y;
        this.convValue=eval(this.conv);
        if(typeof(this.convValue)=="function")
          this.convValue=this.convValue(x,y);
      }
      else
        this.convValue={x:this._x,y:this._y};
  
      this.midilearn=this.getAttr("midilearn",opt.midilearn);
      this.midicc=this.getAttr("midicc",null);
      this.midiController={};
      this.midiMode="normal";
      if(this.midicc) {
          let ch = parseInt(this.midicc.substring(0, this.midicc.lastIndexOf("."))) - 1;
          let cc = parseInt(this.midicc.substring(this.midicc.lastIndexOf(".") + 1));
          this.setMidiController(ch, cc);
      }
      this.setupImage();
      this.digits=0;
      if(window.webAudioControlsMidiManager)
//        window.webAudioControlsMidiManager.updateWidgets();
        window.webAudioControlsMidiManager.addWidget(this);
      this.elem.onclick=(e)=>{e.stopPropagation()};
    }
    disconnectedCallback(){}
    setupImage(){
      this.coltab = this.colors.split(";");
      this.dr=this.direction;
      this.dlen=this.ditchlength;
      if(!this.width)
        this.width=256;
      if(!this.height)
        this.height=256;
      this.knob.style.backgroundSize = "100% 100%";
      this.elem.style.backgroundSize = "100% 100%";
      this.elem.style.width=this.width+"px";
      this.elem.style.height=this.height+"px";
      this.kwidth=this.knobwidth||(this.width*0.15|0);
      this.kheight=this.knobheight||(this.height*0.15|0);
      this.knob.style.width = this.kwidth+"px";
      this.knob.style.height = this.kheight+"px";
      if(!this.src){
        let r=Math.min(this.width,this.height)*0.02;
        let svgbody=
`<svg xmlns="http://www.w3.org/2000/svg" width="${this.width}" height="${this.height}" preserveAspectRatio="none">
<rect x="1" y="1" rx="${r}" ry="${r}" width="${this.width-2}" height="${this.height-2}" fill="${this.coltab[1]}"/></svg>`;
        this.elem.style.backgroundImage = "url(data:image/svg+xml;base64,"+btoa(svgbody)+")";
      }
      else{
        this.elem.style.backgroundImage = "url("+(this.src)+")";
      }
      if(!this.knobsrc){
        let svgthumb=
`<svg xmlns="http://www.w3.org/2000/svg" width="${this.kwidth}" height="${this.kheight}" preserveAspectRatio="none">
<radialGradient id="gr" cx="30%" cy="30%"><stop offset="0%" stop-color="${this.coltab[2]}"/><stop offset="100%" stop-color="${this.coltab[0]}"/></radialGradient>
<rect x="2" y="2" width="${this.kwidth-4}" height="${this.kheight-4}" rx="${this.kwidth*0.5}" ry="${this.kheight*0.5}" fill="url(#gr)"/></svg>`;
        this.knob.style.backgroundImage = "url(data:image/svg+xml;base64,"+btoa(svgthumb)+")";
      }
      else{
        this.knob.style.backgroundImage = "url("+(this.knobsrc)+")";
      }
      this.elem.style.outline=this.outline?"":"none";
      this.redraw();
    }
    redraw() {
      this.digits=0;
      if(this.step && this.step < 1) {
        for(let n = this.step ; n < 1; n *= 10)
          ++this.digits;
      }
      if(this.value<this.min){
        this.value=this.min;
        return;
      }
      if(this.value>this.max){
        this.value=this.max;
        return;
      }
      let range = this.max - this.min;
      let style = this.knob.style;
      style.left=(this.width-this.kwidth)*(this._x-this.min)/(this.max-this.min)+"px"; style.top=(this.height-this.kheight)*(1-(this._y-this.min)/(this.max-this.min))+"px";
      this.sensex=0; this.sensey=1;
    }
    _setX(v){
      v=(Math.round((v-this.min)/this.step))*this.step+this.min;
      this._x=Math.min(this.max,Math.max(this.min,v));
      if(this._x!=this.oldx){
        this.oldx=this._x;
        if(this.conv){
          const x=this._x;
          const y=this._y;
          this.convValue=eval(this.conv);
          if(typeof(this.convValue)=="function")
            this.convValue=this.convValue(x,y);
        }
        else
          this.convValue={x:this._x,y:this._y};
        this.redraw();
        this.showtip(0);
        return 1;
      }
      return 0;
    }
    _setY(v){
      v=(Math.round((v-this.min)/this.step))*this.step+this.min;
      this._y=Math.min(this.max,Math.max(this.min,v));
      if(this._y!=this.oldy){
        this.oldy=this._y;
        if(this.conv){
          const x=this._x;
          const y=this._y;
          this.convValue=eval(this.conv);
          if(typeof(this.convValue)=="function")
            this.convValue=this.convValue(x,y);
        }
        else
          this.convValue={x:this._x,y:this._y};
        this.redraw();
        this.showtip(0);
        return 1;
      }
      return 0;
    }
    setX(v,f){
      if(this._setX(v)&&f)
        this.sendEvent("input"),this.sendEvent("change");
    }
    setY(v,f){
      if(this._setY(v)&&f)
        this.sendEvent("input"),this.sendEvent("change");
    }
    wheel(e) {
      let delta=(this.max-this.min)*0.01;
      delta=e.deltaY>0?-delta:delta;
      if(!e.shiftKey)
        delta*=5;
      if(Math.abs(delta) < this.step)
        delta = (delta > 0) ? +this.step : -this.step;
      this.setValue(+this.value+delta,true);
      e.preventDefault();
      e.stopPropagation();
      this.redraw();
    }
    pointerdown(ev){
      if(!this.enable)
        return;
      let e=ev;
      if(ev.touches){
        e = ev.changedTouches[0];
        this.identifier=e.identifier;
      }
      else {
        if(e.buttons!=1 && e.button!=0)
          return;
      }
      this.elem.focus();
      this.drag=1;
      this.showtip(0);
      let pointermove=(ev)=>{
        let e=ev;
        if(ev.touches){
          for(let i=0;i<ev.touches.length;++i){
            if(ev.touches[i].identifier==this.identifier){
              e = ev.touches[i];
              break;
            }
          }
        }
        if(this.lastShift !== e.shiftKey) {
          this.lastShift = e.shiftKey;
          this.startPosX = e.pageX;
          this.startPosY = e.pageY;
          this.startVal = this.value;
        }
        let offsetX = (e.pageX - this.startPosX);
        let offsetY = (this.startPosY - e.pageY);
        let rc=this.getBoundingClientRect();
        this._setX(this.min+(this.max-this.min)*(e.pageX-rc.x-this.kwidth*.5)/(this.width-this.kwidth));
        this._setY(this.min+(this.max-this.min)*(1-(e.pageY-rc.y-this.kheight*.5)/(this.height-this.kheight)));
        this.sendEvent("input");
        if(e.preventDefault)
          e.preventDefault();
        if(e.stopPropagation)
          e.stopPropagation();
        return false;
      }
      let pointerup=(ev)=>{
        let e=ev;
        if(ev.touches){
          for(let i=0;;){
            if(ev.changedTouches[i].identifier==this.identifier){
              break;
            }
            if(++i>=ev.changedTouches.length)
              return;
          }
        }
        this.drag=0;
        this.showtip(0);
        this.startPosX = this.startPosY = null;
        window.removeEventListener('mousemove', pointermove);
        window.removeEventListener('touchmove', pointermove, {passive:false});
        window.removeEventListener('mouseup', pointerup);
        window.removeEventListener('touchend', pointerup);
        window.removeEventListener('touchcancel', pointerup);
        document.body.removeEventListener('touchstart', preventScroll,{passive:false});
        this.sendEvent("change");
      }
      pointermove(ev);
      let preventScroll=(e)=>{
        e.preventDefault();
      }
      if(e.touches)
        e = e.touches[0];
      if(e.ctrlKey || e.metaKey)
        this.setValue(this.defvalue,true);
      else {
        this.startPosX = e.pageX;
        this.startPosY = e.pageY;
        this.startVal = this.value;
        window.addEventListener('mousemove', pointermove);
        window.addEventListener('touchmove', pointermove, {passive:false});
      }
      window.addEventListener('mouseup', pointerup);
      window.addEventListener('touchend', pointerup);
      window.addEventListener('touchcancel', pointerup);
      document.body.addEventListener('touchstart', preventScroll,{passive:false});
      e.preventDefault();
      e.stopPropagation();
      return false;
    }
  });
} catch(error){
  console.log("webaudio-xypad already defined");
}



  // FOR MIDI LEARN
  class WebAudioControlsMidiManager {
    constructor(){
      this.midiAccess = null;
      this.listOfWidgets = [];
      this.listOfExternalMidiListeners = [];
      this.updateWidgets();
      this.initWebAudioControls();
    }
    addWidget(w){
      this.listOfWidgets.push(w);
    }
    updateWidgets(){
//      this.listOfWidgets = document.querySelectorAll("webaudio-knob,webaudio-slider,webaudio-switch");
    }
    initWebAudioControls() {
      if(navigator.requestMIDIAccess) {
        navigator.requestMIDIAccess().then(
          (ma)=>{this.midiAccess = ma,this.enableInputs()},
          (err)=>{ console.log("MIDI not initialized - error encountered:" + err.code)}
        );
      }
    }
    enableInputs() {
      let inputs = this.midiAccess.inputs.values();
      console.log("Found " + this.midiAccess.inputs.size + " MIDI input(s)");
      for(let input = inputs.next(); input && !input.done; input = inputs.next()) {
        console.log("Connected input: " + input.value.name);
        input.value.onmidimessage = this.handleMIDIMessage.bind(this);
      }
    }
    midiConnectionStateChange(e) {
      console.log("connection: " + e.port.name + " " + e.port.connection + " " + e.port.state);
      enableInputs();
    }

    onMIDIStarted(midi) {
      this.midiAccess = midi;
      midi.onstatechange = this.midiConnectionStateChange;
      enableInputs(midi);
    }
    // Add hooks for external midi listeners support
    addMidiListener(callback) {
      this.listOfExternalMidiListeners.push(callback);
    }
    getCurrentConfigAsJSON() {
      return currentConfig.stringify();
    }
    handleMIDIMessage(event) {
      this.listOfExternalMidiListeners.forEach(function (externalListener) {
        externalListener(event);
      });
      if(((event.data[0] & 0xf0) == 0xf0) || ((event.data[0] & 0xf0) == 0xb0 && event.data[1] >= 120))
        return;
      for(let w of this.listOfWidgets) {
        if(w.processMidiEvent)
          w.processMidiEvent(event);
      }
      if(opt.mididump)
        console.log(event.data);
    }
    contextMenuOpen(e,knob){
      if(!this.midiAccess)
        return;
      let menu=document.getElementById("webaudioctrl-context-menu");
      menu.style.left=e.pageX+"px";
      menu.style.top=e.pageY+"px";
      menu.knob=knob;
      menu.classList.add("active");
      menu.knob.focus();
//      document.activeElement.onblur=this.contextMenuClose;
      menu.knob.addEventListener("keydown",this.contextMenuCloseByKey.bind(this));
    }
    contextMenuCloseByKey(e){
      if(e.keyCode==27)
       this.contextMenuClose();
    }
    contextMenuClose(){
      let menu=document.getElementById("webaudioctrl-context-menu");
      menu.knob.removeEventListener("keydown",this.contextMenuCloseByKey);
      menu.classList.remove("active");
      let menuItemLearn=document.getElementById("webaudioctrl-context-menu-learn");
      menuItemLearn.innerHTML = 'Learn';
      menu.knob.midiMode = 'normal';
    }
    contextMenuLearn(){
      let menu=document.getElementById("webaudioctrl-context-menu");
      let menuItemLearn=document.getElementById("webaudioctrl-context-menu-learn");
      menuItemLearn.innerHTML = 'Listening...';
      menu.knob.midiMode = 'learn';
    }
    contextMenuClear(e){
      let menu=document.getElementById("webaudioctrl-context-menu");
      menu.knob.midiController={};
      this.contextMenuClose();
    }
  }
  if(window.UseWebAudioControlsMidi||opt.useMidi)
    window.webAudioControlsMidiManager = new WebAudioControlsMidiManager();
}
