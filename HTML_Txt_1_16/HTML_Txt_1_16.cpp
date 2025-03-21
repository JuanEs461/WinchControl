#include "HTML_Txt_1_16.h"

//Each function can contain a maximum of 2326 characters

const char* HTMLConnect(){
  return "HTTP/1.1 200 OK\
          Content-Type: text/html\
          Connection: close";
}

const char* HTMLGlobalVariables(){
	return "let array = [0];\n\
			let printerval;\n\
			let stopState = 1;\n\
			printerval = setInterval(() => sendPosition(), 150);\
			let count = 0";
}

const char* HTMLStartPressed(){
  return "  function StartPressed() { \n\
              var request = new XMLHttpRequest(); \n\
              request.onreadystatechange = function() { \n\
                if (this.readyState == 4) { \n\
                 if (this.status == 200) { \n\
                  if (this.responseText != null){ \n\
					if(stopState == 1){\n\
                    document.getElementById(\"startLabel\").innerHTML = this.responseText; \n\
					document.getElementById(\"startLabel\").className = \"box Left Basic\";\n\
					document.getElementById(\"speedValue\").innerHTML = \"\"; \n\
					document.getElementById(\"speedValue\").className = \"box Right Basic\";\n\
					document.getElementById(\"speedSlider\").value = 0; \n\
					document.getElementById(\"speedSlider\").dispatchEvent(new Event('change'));\n\
					stopState = 0;\n\
					} else {\n\
						document.getElementById(\"startLabel\").innerHTML = this.responseText;\n\
					}}}}}\n\
              request.open(\"GET\", \"startButtonPressed\", true);\n\
              request.send();}";
}

const char* HTMLStopPressed(){
  return "  function StopPressed() { \n\
			  stopState = 1;\n\
              var request = new XMLHttpRequest(); \n\
              request.onreadystatechange = function() { \n\
               if (this.readyState == 4) { \n\
                if (this.status == 200) { \n\
                 if (this.responseText != null){ \n\
                 document.getElementById(\"startLabel\").innerHTML = this.responseText; \n\
				 document.getElementById(\"startLabel\").className = \"box Left Basic\";\n\
                 document.getElementById(\"outLabel\").innerHTML = \"\"; \n\
                 document.getElementById(\"speedValue\").innerHTML = \"\"; \n\
				 document.getElementById(\"speedValue\").className = \"box Right Basic\";\n\
                 document.getElementById(\"speedSlider\").value = 0; \n\
				 document.getElementById(\"speedSlider\").dispatchEvent(new Event('change'));\n\
				 array = [];\n\
				 array[1] = 0;\n\
                 }}}}\n\
              request.open(\"GET\", \"stopButtonPressed\", true);\n\
              request.send();}";
}

const char* LimitPressed(){
	return "function LimitPressed() {\n\
	var request = new XMLHttpRequest();\n\
	request.onreadystatechange = function() {\n\
		if (this.readyState == 4) {\n\
		if (this.status == 200) {\n\
		if (this.responseText != null){\n\
			myArray = this.responseText.split(\"|\");\n\
			document.getElementById(\"limitToggle\").innerHTML = myArray[0];\n\
			document.getElementById(\"limitToggle\").className = myArray[1];\n\
	}}}}\n\
	request.open(\"GET\", \"limitTogglePressed\", true);\n\
	request.send();}";
}
const char* HTMLBeforeClose() {
  return "  function BeforeClose() { \n\
  			stopState = 1;\n\
			count = 0;\n\
  			document.getElementById(\"startLabel\").innerHTML = \"Stopped\"; \n\
			document.getElementById(\"startLabel\").className = \"box Left Basic\";\n\
			document.getElementById(\"outLabel\").innerHTML = \"\"; \n\
			document.getElementById(\"speedValue\").innerHTML = \"\"; \n\
			document.getElementById(\"speedValue\").className = \"box Right Basic\";\n\
			document.getElementById(\"speedSlider\").value = 0;\n\
			document.getElementById(\"speedSlider\").dispatchEvent(new Event('change'));\n\
			array = [];\n\
			array[1] = 0;\n\
            var request = new XMLHttpRequest(); \n\
            request.open(\"GET\", \"/beforeClose\", true); \n\
            request.send(null); \n\
			return \"Something clever\";}";
}

const char* SetMinLimit() {
	return "function SetMinLimit() {\n\
	let input = document.getElementById(\"minLimit\");\n\
	if(parseInt(input.value) > parseInt(input.max)){\n\
		input.value = input.max;\n\
	}else if (parseInt(input.value) < parseInt(input.min)){\n\
		input.value = input.min;\n\
	}\n\
	var request = new XMLHttpRequest();\n\
	var url = \"/minLimitSet=\" + document.getElementById(\"minLimit\").value;\n\
	request.open(\"GET\", url, true);\n\
	request.send();}";
}

const char* SetMaxLimit() {
	return "function SetMaxLimit() {\n\
	let input = document.getElementById(\"maxLimit\");\n\
	if(parseInt(input.value) > parseInt(input.max)){\n\
		input.value = input.max;\n\
	} else if (parseInt(input.value) < parseInt(input.min)){\n\
		input.value = input.min;\n\
	}\n\
	var request = new XMLHttpRequest();\n\
	var url = \"/maxLimitSet=\" + document.getElementById(\"maxLimit\").value;\n\
	request.open(\"GET\", url, true);\n\
	request.send();}";
}
	
const char* sliderPreProcessing(){
	return " function sliderPreProcessing(){\n\
				if(stopState == 0){\n\
				var slider = document.getElementById(\"speedSlider\");\n\
				array.push(slider.value);\n\
				}\n\
		}";
}


const char* HTMLSendPosition(){
  return "  function sendPosition(){\n\
				if(array.length > 1){\n\
				array.shift();\n\
				var request = new XMLHttpRequest();\n\
				var url = \"/speedSlider=\" + array[0];\n\
				request.open(\"GET\", url, true);\n\
				request.send();\n\
				request.onreadystatechange = function() { \n\
					if (this.readyState == 4) { \n\
						if (this.status == 200) { \n\
						if (this.responseText != null){ \n\
							let data = this.responseText;\n\
							let myArray = data.split(\"|\");\n\
							if(myArray[0] == \"minErr\"){\n\
								document.getElementById(\"startLabel\").innerHTML = \"Minimum amount of cable payed in!\";\n\
								document.getElementById(\"startLabel\").className = \"box Left Red\";\n\
								document.getElementById(\"speedValue\").innerHTML = \"\"; \n\
								document.getElementById(\"speedValue\").className = \"box Right Red\";\n\
							} else if(myArray[0] == \"maxErr\"){\n\
								document.getElementById(\"startLabel\").innerHTML = \"Maximum amount of cable payed out!\";\n\
								document.getElementById(\"startLabel\").className = \"box Left Red\";\n\
								document.getElementById(\"speedValue\").innerHTML = \"\"; \n\
								document.getElementById(\"speedValue\").className = \"box Right Red\";\n\
							} else {\n\
							document.getElementById(\"startLabel\").innerHTML = myArray[0];\n\
							document.getElementById(\"outLabel\").innerHTML = myArray[1];\n\
							document.getElementById(\"speedValue\").innerHTML = myArray[2]; \n\
							}\n\
						}}}} \n\
				}}";
}

const char* HTMLReadReload(){
	return "function ReadReload() {\n\
			  var reloadRequest = \"\";\n\
			  var request = new XMLHttpRequest(); \n\
			  request.onreadystatechange = function() {\n\
				if(this.readyState == 4){\n\
					if(this.status == 200) {\n\
						if(this.responseText != null) {\n\
							reloadRequest = this.responseText;\n\
							if(reloadRequest == \"Reload\") {\n\
								window.location.reload();\n\
							} else if(reloadRequest == \"timeout\") {\n\
								document.getElementById(\"startLabel\").innerHTML = \"Stopped\"; \n\
								document.getElementById(\"outLabel\").innerHTML = \"\"; \n\
								document.getElementById(\"speedValue\").innerHTML = \"\"; \n\
								document.getElementById(\"speedSlider\").value = 0; \n\
								document.getElementById(\"speedSlider\").dispatchEvent(new Event('change'));\n\
								stopState = 1;\n\
								count = 0;\n\
								array = [];\n\
								array[1] = 0;\n\
							}\n\
				}}}}\n\
				request.open(\"GET\", \"reload_request\", true); \n\
				request.send(null);\n\
				GetOutputData();\n\
				setTimeout(\"ReadReload()\", 300);\
				}";
}
const char* HTMLGetOutputData(){
	return " function GetOutputData() { \n\
var request = new XMLHttpRequest(); \n\
request.onreadystatechange = function() {\n\
if (this.readyState == 4) { \n\
 if(this.status == 200) { \n\
  if (this.responseText != null) { \n\
	let received = this.responseText; \n\
	let myArray = received.split(\"|\"); \n\
	if (myArray[0] == \"eStop pressed\") {\n\
	document.getElementById(\"eBox\").innerHTML = \"E-STOP PRESSED!\";\n\
	document.getElementById(\"eBox\").className = \"Full Red\";\n\
	document.getElementById(\"startLabel\").innerHTML = \"Stopped\";\n\
	document.getElementById(\"outLabel\").innerHTML = \"\"; \n\
	document.getElementById(\"speedValue\").innerHTML = \"\"; \n\
	document.getElementById(\"speedSlider\").value = 0; \n\
	document.getElementById(\"speedSlider\").dispatchEvent(new Event('change'));\n\
	stopState = 1;\n\
	array = [];\n\
	array[1] = 0;\n\
	} else if (myArray[0] == \"eStop released\") {\n\
	document.getElementById(\"eBox\").innerHTML = \"Ready to run\";\n\
	document.getElementById(\"eBox\").className = \"Full Green\";\n\
	}\n\
	if (myArray[3] == \"minPayErr\"){\n\
	if(array[0] != 0){\n\
	array[1] = 0;\n\
	}\n\
	document.getElementById(\"startLabel\").innerHTML = \"Minimum amount of cable payed in!\";\n\
	document.getElementById(\"startLabel\").className = \"box Left Red\";\n\
	document.getElementById(\"outLabel\").innerHTML = \"\"; \n\
	document.getElementById(\"speedValue\").innerHTML = \"\"; \n\
	document.getElementById(\"speedSlider\").value = 0; \n\
	document.getElementById(\"speedValue\").className = \"box Right Red\";\n\
	document.getElementById(\"speedSlider\").dispatchEvent(new Event('change'));\n\
	stopState = 1;\n\
	} ";
}

const char* HTMLGetOutputData2(){
return "else if (myArray[3] == \"maxPayErr\"){\n\
	if(array[0] != 0){\n\
	array[1] = 0;\n\
	}\n\
	document.getElementById(\"startLabel\").innerHTML = \"Maximum amount of cable payed out!\";\n\
	document.getElementById(\"startLabel\").className = \"box Left Red\";\n\
	document.getElementById(\"outLabel\").innerHTML = \"\"; \n\
	document.getElementById(\"speedValue\").innerHTML = \"\"; \n\
	document.getElementById(\"speedValue\").className = \"box Right Red\";\n\
	document.getElementById(\"speedSlider\").value = 0; \n\
	document.getElementById(\"speedSlider\").dispatchEvent(new Event('change'));\n\
	stopState = 1;\n\
	}\n\
	if(count == 0){\n\
	document.getElementById(\"minLimit\").value = myArray[4];\n\
	document.getElementById(\"maxLimit\").value = myArray[5];\n\
	document.getElementById(\"limitToggle\").innerHTML = myArray[6];\n\
	document.getElementById(\"limitToggle\").className = myArray[7];\n\
	count = 1;\n\
	}\n\
	document.getElementById(\"payoutMeters\").innerHTML = myArray[1] + \"m\";\n\
	document.getElementById(\"payoutSpeed\").innerHTML = myArray[2] + \"m/s\";\n\
}}}}\n\
request.open(\"GET\", \"/output_ajax_switch\", true); \n\
request.send(null); }";
}

const char* HTMLInputReadReload(){
	return "function ReadReload() {\n\
			  var reloadRequest = \"\";\n\
			  var request = new XMLHttpRequest(); \n\
			  request.onreadystatechange = function() {\n\
				if(this.readyState == 4){\n\
					if(this.status == 200) {\n\
						if(this.responseText != null) {\n\
							reloadRequest = this.responseText;\n\
							if(reloadRequest == \"Reload\") {\n\
								window.location.reload();\n\
							}\n\
				}}}}\n\
				request.open(\"GET\", \"reload_request\", true); \n\
				request.send(null);\n\
				GetSwitchAnalogData();\n\
				setTimeout(\"ReadReload()\", 300); }";
}

const char* HTMLGetSwitchAnalogData(){
	return " function GetSwitchAnalogData() { \n\
              var request = new XMLHttpRequest(); \n\
              request.onreadystatechange = function() {\n\
                if (this.readyState == 4) { \n\
				 if(this.status == 200) { \n\
                  if (this.responseText != null) { \n\
                    let recieved = this.responseText; \n\
					let myArray = recieved.split(\"|\"); \n\
					document.getElementById(\"startBox\").innerHTML = myArray[0];\n\
					document.getElementById(\"startBox\").className = myArray[1];\n\
					document.getElementById(\"fwdRevBox\").innerHTML = myArray[2];\n\
					document.getElementById(\"fwdRevBox\").className = myArray[3];\n\
					document.getElementById(\"speedBox\").innerHTML = myArray[4];\n\
					document.getElementById(\"speedBox\").className = myArray[5];\n\
					document.getElementById(\"payoutMeters\").innerHTML = myArray[6] + \"m\";\n\
					document.getElementById(\"payoutSpeed\").innerHTML = myArray[7] + \"m/s\";\n\
					document.getElementById(\"limit\").innerHTML = myArray[8];\n\
					document.getElementById(\"limit\").className = myArray[9];\n\
				}}}}\n\
              request.open(\"GET\", \"/ajax_switch\", true); \n\
			  request.send(null); }";
}

const char* HTMLSliderScript(){
		return "let inputs = document.querySelectorAll(\"input[type=range]\");\n\
inputs.forEach((range) => {\n\
  range.addEventListener(\"change\", () => {\n\
    styleRange(range);\n\
  });\n\
  range.addEventListener(\"input\", () => {\n\
    styleRange(range);\n\
  });\n\
});\n\
\
function styleRange(input) {\n\
  setWidth(input);\n\
  setPosition(input);\n\
\
  function setWidth(input) {\n\
    input.parentElement.style.setProperty(\n\
      \"--before-width\",\n\
      `${calcBeforeWidth(input)}px`,\n\
    );\n\
  }\n\
\
  function setPosition(input) {\n\
    input.parentElement.style.setProperty(\n\
      \"--before-left\",\n\
      `${calcPositionLeft(input)}px`,\n\
    );\n\
  }\n\
\
  function calcBeforeWidth(input) {\n\
    return (Math.abs(input.value) * widthPerStep(input));\n\
  }\n\
\
  function widthPerStep(input) {\n\
    const style = window.getComputedStyle(input);\n\
    const totalWidth = parseFloat(style.getPropertyValue(\"width\"));\n\
    return totalWidth / numberSteps(input);\n\
  }\n\
\
  function numberSteps(input) {\n\
    if (input.min < input.max) return Math.abs(input.min) + Math.abs(input.max);\n\
    return Math.abs(input.max) - Math.abs(input.min);\n\
  }\n\
\
  function calcPositionLeft(input) {\n\
    const style = window.getComputedStyle(input);\n\
    const totalWidth = parseFloat(style.getPropertyValue(\"width\"));\n\
    const inputHeight = parseFloat(style.getPropertyValue(\"height\"));\n\
\
    if (input.getAttribute(\"value\") >= input.value)\
      return (totalWidth / 2) + (inputHeight / 2) - calcBeforeWidth(input);\n\
    return totalWidth / 2;\n\
  }\n\
}";
}

const char* HTMLOutputHeader(){
  return "  <title>Winch Control</title> \
            <style>\
            .button {\
              border: none;\
              color: white;\
              padding: 15px 30px;\
              text-decoration: none;\
              font-size: 25px;\
              margin: 4px 10px;\
              cursor: pointer;\
            }\
            .buttonGreen {background-color: #4CAF50;}\
            .buttonGreen:active {background-color: #2F9E34;}\
            .buttonRed {background-color: #FC0303;}\
            .buttonRed:active {background-color: #CF0808;}\
            .buttonBasic {background-color: #000000;}\
            .buttonBasic:active {background-color: #595757;}\
";}
const char* HTMLOutputFormat(){
	return "h1 {text-align:center;}\
		span {margin: 10px 3.4%; font-size: 20px;}\
		.centered {text-align:center;\
			color: black;\
			margin: 20px;\
		}\
		.control-panel{position:relative;\
			text-align:center; margin:auto;\
			min-width: 615px; white-space: nowrap;}\
		.start-position{margin-right: 5px; display:inline-block}\
		.stop-position{margin-left: 5px; display:inline-block}\
		div.box {\
			padding: 15px;\
			text-align: center;\
			font-size: 20px;\
		}\
		.Red {background-color: #FC0303;}\
		.Green {background-color: #4CAF50;}\
		label.box {\
			text-align: center;\
			display: inline;\
			font-size: 30px;\
			color: white;\
			}\
		.Full {padding: 10px; color: white;}\
		.Middle {padding: 10px 0px 10px 0px;}\
		.Left {padding: 10px 0px 10px 10px;}\
		.Right {padding: 10px 10px 10px 0px;}\
		.Basic {background-color: black;}\
		.Yellow {background-color: orange;}\
		.Clear {border: 0px blue;}\
		.labelLeft {position: relative; right: 23px; top: -20px; display: inline-block; width: 0;}\
		.labelRight {position: relative; left: -105px; top: -20px; display: inline-block; width: 0;}";
}

const char* HTMLSliderCSS() {
	return ".input-wrapper {\
  padding: 10px 0;\
  box-sizing: border-box;\
  position: relative;\
  min-height: 60px;\
  display: grid;\
  place-items: center;\
}\
input[type=\"range\"],\
input[type=\"range\"] + datalist {\
  --thumb-width: 16px;\
  --thumb-height: 16px;\
}\
body {\
  font-size: 16px;\
  margin: 0;\
  background-color: LightBlue;\
  min-width:700px;\
}\
.parent-input {\
  --input-height: 1rem;\
  --input-color: blue;\
  position: relative;\
  display: flex;\
}\
input[type=\"range\"] {\
  width: 100%;\
  max-width: 100%;\
  appearance: none;\
  height: var(--input-height);\
  border-radius: var(--input-height);\
  accent-color: green;\
  width:90vw;\
}\
/* style Chrome range and thumb */\
.range-control[type=\"range\"]::-webkit-slider-runnable-track {\
  -webkit-appearance: none;\
  height: 10px;\
  position: absolute;\
  top: 0px;\
  width: 100%;\
  left: 0;\
  right: 0;\
  cursor: pointer;\
}\
\
.range-control[type=\"range\"]::-webkit-slider-thumb {\
  background-color: blue;\
  border: 1px solid #000000;\
  box-sizing: border-box;\
  height: var(--thumb-height) ;\
  width: var(--thumb-width);\
  -webkit-appearance: none;\
  border-radius: 50%;\
  cursor: pointer;\
  margin-top:2px;\
}\
\
.parent-input::before {\
  content: \"\";\
  width: var(--before-width, 0);\
  height: var(--input-height, 0);\
  border-radius: var(--input-height);\
  pointer-events: none;\
  position: absolute;\
  background-color: blue;\
  top: 50%;\
  transform: translateY(-50%);\
  left: var(--before-left, 0);\
}";
}
const char* HTMLSliderList(){
return ".range-list {\
  margin-top: -12px;\
  box-sizing: border-box;\
  width: 100%;\
  z-index: 0;\
  display: flex;\
  justify-content: space-between;\
  position: absolute;\
  top: 30px;\
  left: 0;\
  right: 0;\
}\
.range-list option {\
  --option-width: calc((100% - 12px) / (var(--list-length) - 1));\
  display: inline-block;\
  padding-top: calc(var(--thumb-height) / 2);\
  width: var(--option-width);\
  text-align: center;\
  position: relative;\
  &::before {\
    content: \"\";\
    position: absolute;\
    border-left: 1px;\
    width: 1px;\
    height: 5px;\
    border-left: 1px solid #000000;\
    top: 0;\
    left: 50%;\
    margin-left: -var(--option-width);\
  }\
}\
\
.range-list option:first-child {\
  width: calc(\
    (100% - var(--thumb-width)) / ((var(--list-length) - 1) * 2) +\
      var(--thumb-width) / 2\
  );\
  text-align: left;\
  &::before {\
    left: 7px;\
    margin-left: unset;\
  }\
}\
.range-list option:last-child {\
  width: calc((100% - 12px) / ((var(--list-length) - 1) * 2) + 6px);\
  text-align: right;\
  &::before {\
    left: unset;\
    margin-left: unset;\
    right: 7px;\
  }\
}\
.number {\
	  vertical-align:bottom;\
	  width:60px;\
	  padding: 4px 10px;\
	  border: 1px solid #bbb;\
	  border-radius: 5px;\
	  color: black;\
	}\
.left {margin: 0 30px 10px 0;}\
.right { margin: 0 0 10px 30px;}";
}
const char* HTMLOutputBody(){
  return "<body onbeforeunload=\"return BeforeClose()\" onload=\"ReadReload()\"> \n\
<h1>AGO Environmental Winch Controls</h1>\n\
<div class=\"box\"><span class=\"Full Basic\">Mode: Computer Controls</span></div>\
<div class=\"box\"><span class=\"Full Green\" id=\"eBox\" style=\"border-style: solid; border-color: black\">Ready to run</span></div>\
<div class = \"control-panel\"><label for=\"maxLimit\" class=\"labelLeft\">Set MAX Limit in M</label><input type=\"number\" class=\"number left\" id=\"maxLimit\" min=\"0.0\" max=\"9999.9\"step=\"0.5\" onchange=\"SetMaxLimit()\"><!--minLimitLimitSet-->\n\
<div class=\"start-position\"><button type=\"button\" class=\"button buttonGreen\" id=\"startButton\" onclick=\"StartPressed()\">Start</button></div>\n\
<div class=\"stop-position\"><button type=\"button\" class=\"button buttonRed\" id=\"stopButton\" onclick=\"StopPressed()\">Stop</button></div>\n\
<input type=\"number\" class=\"number right\" id=\"minLimit\" min=\"0.0\" max=\"9999.9\" step=\"0.5\" onchange=\"SetMinLimit()\"><!--minLimitLimitSet--><label for=\"minLimit\" class=\"labelRight\">Set MIN Limit in M</label></div>\n\
<div class=\"input-wrapper\"><div class=\"parent-input\">\
<input type=\"range\" id=\"speedSlider\" min=\"-120\" max=\"120\" list=\"range-list\" step=\"5\" class=\"range-control\" oninput=\"sliderPreProcessing()\" onchange=\"sliderPreProcessing()\"/>\
<datalist id=\"range-list\" class=\"range-list\" style=\"--list-length: 13;\">\
<option value=\"-120\">100</option><option>80</option><option>60</option><option>40</option><option>20</option><option>OUT</option><option>&nbsp</option>\
<option>IN</option><option>20</option><option>40</option><option>60</option><option>80</option><option value=\"\">100</option></datalist>\
</div></div>\
<div class = \"centered\">\n\
<label for=\"startButton\" id = \"startLabel\" class=\"box Left Basic\">Stopped</label>\n\
<label for=\"outButton\" id = \"outLabel\" class=\"box Middle Basic\"></label>\n\
<label for=\"speedSlider\" id = \"speedValue\" class=\"box Right Basic\"></label>\n\
</div>";
}

const char* HTMLOutputBody1() {
	return"<div class = \"centered\">\n\
<label id=\"payoutM\" class=\"box Full Basic\">Cable Payout: </label>\n\
<label id=\"payoutMeters\" class=\"box Full Basic\">0.00m</label>\n\
<label id=\"payoutS\" class=\"box Full Basic\">Cable Speed: </label>\n\
<label id=\"payoutSpeed\" class=\"box Full Basic\">0.00m/s</label>\n\
</div>\
<div class = \"centered\">\n\
<button type=\"button\" class=\"button buttonGreen\" id=\"limitToggle\" onclick=\"LimitPressed()\">Limits ON</button>\n\
</div>";
}

const char* HTMLInputHeader(){
	return "<title>Manual Winch Mode</title>\
            	<style>\
                span.box {\
                padding: 10px;\
                text-align: center;\
                display: inline;\
                font-size: 30px;\
				color: white;\
                }\
                .Green {background-color: green;}\
                .Red {background-color: red;}\
                .Basic {background-color: black;}\
                .Yellow {background-color: orange;}\
				.Clear {border: 0px blue;}\
				h1 {text-align:center;}\
				p {text-align:center;\
				font-size: 30px;\
				color: white;\}\
				body{\
					background-color: aquamarine;\
					min-width: 700px;\
				}\
				.Right {padding: 10px 10px 10px 0px;}\
              </style>";
}

const char* HTMLInputBody(){
  return "  <h1>AGO Environmental Winch Monitoring</h1>\
			<p><span class=\"box Basic\">Mode: Manual Monitoring</span></p><br></br>\
			<p><span class=\"box Red\" id=\"startBox\">Stopped</span></p><br></br>\
			<p><span class=\"box Clear\" id=\"fwdRevBox\"></span><span class=\"box Clear\" id=\"speedBox\"></span></p><br></br>\
			<p><span id=\"payoutM\" class=\"box Basic\">Cable Payout: </span>\n\
		  <span id=\"payoutMeters\" class=\"Basic Right\">0.00m</span>\n\
		  <span id=\"payoutS\" class=\"box Basic\">Cable Speed: </span>\n\
		  <span id=\"payoutSpeed\" class=\"Basic Right\">0.00m/s</span></p>\n\
		  <p><span id=\"limit\" class=\"box Clear\"></span></p>";
}