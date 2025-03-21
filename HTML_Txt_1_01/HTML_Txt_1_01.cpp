#include "HTML_Txt_1_01.h"

const char* HTMLConnect(){
  return "HTTP/1.1 200 OK\
          Content-Type: text/html\
          Connection: keep-alive";
}

const char* HTMLGlobalVariables(){
	return "let array = [0];\n\
			let printerval;\n\
			printerval = setInterval(() => sendPosition(), 100);";
}


const char* HTMLStartPressed(){
  return "  function StartPressed() { \n\
              var request = new XMLHttpRequest(); \n\
              request.onreadystatechange = function() { \n\
                if (this.readyState == 4) { \n\
                 if (this.status == 200) { \n\
                  if (this.responseText != null){ \n\
                    document.getElementById(\"startLabel\").innerHTML = this.responseText; \n\
                 }}}}\n\
              request.open(\"GET\", \"startButtonPressed\", true);\n\
              request.send();}";
}

const char* HTMLStopPressed(){
  return "  function StopPressed() { \n\
			  count += 1;\n\
			  console.log(count);\n\
              var request = new XMLHttpRequest(); \n\
              request.onreadystatechange = function() { \n\
               if (this.readyState == 4) { \n\
                if (this.status == 200) { \n\
                 if (this.responseText != null){ \n\
                 document.getElementById(\"startLabel\").innerHTML = this.responseText; \n\
                 document.getElementById(\"outLabel\").innerHTML = \"\"; \n\
                 document.getElementById(\"speedValue\").innerHTML = \"\"; \n\
                 document.getElementById(\"speedSlider\").value = \"0\"; \n\
                 document.getElementById(\"rangenumber\").value = \"0\"; \n\
				 array[1] = 0;\n\
                 }}}}\n\
              request.open(\"GET\", \"stopButtonPressed\", true);\n\
              request.send();}";
}

const char* HTMLOutPressed(){
  return "  function OutPressed() { \n\
              var request = new XMLHttpRequest(); \n\
              request.onreadystatechange = function() { \n\
                if (this.readyState == 4) { \n\
                 if (this.status == 200) { \n\
                  if (this.responseText != null){ \n\
					let data = this.responseText;\n\
					if(data == \"zero\"){\n\
						document.getElementById(\"speedSlider\").value = 0;\n\
						document.getElementById(\"rangenumber\").value = 0;\n\
						document.getElementById(\"startLabel\").innerHTML = \"Running \";\n\
						document.getElementById(\"outLabel\").innerHTML = \"Out at \";\n\
						array[1] = 0;\n\
					} else { \n\
						console.log(data);\n\
						let myArray = data.split(\"!\"); \n\
						document.getElementById(\"startLabel\").innerHTML = myArray[0];\n\
						document.getElementById(\"outLabel\").innerHTML = myArray[1]; \n\
					}\n\
                 }}}}\n\
              request.open(\"GET\", \"outButtonPressed\", true);\n\
              request.send();}";
}

const char* HTMLJogOutPressed(){
	return " function JogOutPressed() {\n\
			 var request = new XMLHttpRequest(); \n\
			 request.onreadystatechange = function() {\n\
				if (this.readyState == 4) {\n\
					if (this.status == 200) {\n\
						if (this.responseText != null){\n\
							let data = this.responseText;\n\
							console.log(data);\n\
							if(data == \"zero\"){\n\
								document.getElementById(\"speedSlider\").value = 0;\n\
								document.getElementById(\"rangenumber\").value = 0;\n\
								document.getElementById(\"startLabel\").innerHTML = \"Jogging \"; \n\
								document.getElementById(\"outLabel\").innerHTML = \"Out at \";\n\
								array[1] = 0;\n\
							} else if(data == \"Out\") { \n\
								console.log(\"in in\");\n\
								document.getElementById(\"startLabel\").innerHTML = \"Jogging \"; \n\
								document.getElementById(\"outLabel\").innerHTML = \"Out at \"; \n\
								document.getElementById(\"speedValue\").innerHTML = array[0] + \"%\";\n\
							}\n\
			}	}	}	}\n\
			 request.open(\"GET\", \"/jogOutPressed\", true);\n\
			 request.send(null);}";
}

const char* HTMLJogOutReleased(){
	return "  function JogOutReleased() { \n\
              var request = new XMLHttpRequest(); \n\
              request.onreadystatechange = function() { \n\
               if (this.readyState == 4) { \n\
                if (this.status == 200) { \n\
                 if (this.responseText != null){ \n\
				 if (this.responseText == \"\"){ \n\
				 console.log(\"nothing happened out\");\n\
				 } else {\n\
                 document.getElementById(\"startLabel\").innerHTML = this.responseText; \n\
                 document.getElementById(\"outLabel\").innerHTML = \"\"; \n\
				 document.getElementById(\"speedValue\").innerHTML = array[0] + \"%\";\n\
                 }}}}}\n\
              request.open(\"GET\", \"jogOutReleased\", true);\n\
              request.send();}";
}

const char* HTMLInPressed(){
  return "  function InPressed() { \n\
              var request = new XMLHttpRequest(); \n\
              request.onreadystatechange = function() { \n\
                if (this.readyState == 4) { \n\
                 if (this.status == 200) { \n\
                   if (this.responseText != null){ \n\
                    let data = this.responseText;\n\
					if(data == \"zero\"){\n\
						document.getElementById(\"speedSlider\").value = 0;\n\
						document.getElementById(\"rangenumber\").value = 0;\n\
						document.getElementById(\"startLabel\").innerHTML = \"Running \";\n\
						document.getElementById(\"outLabel\").innerHTML = \"In at \";\n\
						array[1] = 0;\n\
					} else { \n\
						console.log(data);\n\
						let myArray = data.split(\"!\"); \n\
						document.getElementById(\"startLabel\").innerHTML = myArray[0];\n\
						document.getElementById(\"outLabel\").innerHTML = myArray[1]; \n\
					}\n\
                 }}}}\n\
              request.open(\"GET\", \"inButtonPressed\", true);\n\
              request.send();}";
}

const char* HTMLJogInPressed(){
	return " function JogInPressed() {\n\
			 var request = new XMLHttpRequest(); \n\
			 request.onreadystatechange = function() {\n\
				if (this.readyState == 4) {\n\
					if (this.status == 200) {\n\
						if (this.responseText != null){\n\
							let data = this.responseText;\n\
							console.log(data);\n\
							if(data == \"zero\"){\n\
								document.getElementById(\"speedSlider\").value = 0;\n\
								document.getElementById(\"rangenumber\").value = 0;\n\
								document.getElementById(\"startLabel\").innerHTML = \"Jogging \"; \n\
								document.getElementById(\"outLabel\").innerHTML = \"In at \";\n\
								array[1] = 0;\n\
							} else if(data == \"In\") { \n\
								console.log(\"in in\");\n\
								document.getElementById(\"startLabel\").innerHTML = \"Jogging \"; \n\
								document.getElementById(\"outLabel\").innerHTML = \"In at \"; \n\
								document.getElementById(\"speedValue\").innerHTML = array[0] + \"%\";\n\
							}\n\
			}	}	}	}\n\
			 request.open(\"GET\", \"/jogInPressed\", true);\n\
			 request.send(null);}";
}

const char* HTMLJogInReleased(){
	return "  function JogInReleased() { \n\
              var request = new XMLHttpRequest(); \n\
              request.onreadystatechange = function() { \n\
               if (this.readyState == 4) { \n\
                if (this.status == 200) { \n\
                 if (this.responseText != null){ \n\
				 if(this.responseText == \"\"){\n\
					 console.log(\"nothing happened\");\n\
				 }else{\n\
                 document.getElementById(\"startLabel\").innerHTML = this.responseText; \n\
                 document.getElementById(\"outLabel\").innerHTML = \"\"; \n\
				 document.getElementById(\"speedValue\").innerHTML = array[0] + \"%\";\n\
                 }}}}}\n\
              request.open(\"GET\", \"jogInReleased\", true);\n\
              request.send();}";
}

const char* HTMLBeforeClose() {
  return "  function BeforeClose() { \n\
  			document.getElementById(\"startLabel\").innerHTML = \"Stopped\"; \n\
			document.getElementById(\"outLabel\").innerHTML = \"\"; \n\
			document.getElementById(\"speedValue\").innerHTML = \"\"; \n\
			document.getElementById(\"speedSlider\").value = 0;\n\
			document.getElementById(\"rangenumber\").value = 0;\n\
			array[1] = 0;\n\
            var request = new XMLHttpRequest(); \n\
            request.open(\"GET\", \"/beforeClose\", true); \n\
            request.send(null); \n\
			return \"\"; }";
}

const char* sliderPreProcessing(){
	return " function sliderPreProcessing(){\n\
				var slider = document.getElementById(\"speedSlider\");\n\
				var number = document.getElementById(\"rangenumber\");\n\
				slider.value = number.value;\n\
				array.push(number.value);\n\
		}";
}

const char* HTMLSendPosition(){
  return "  function sendPosition(){\n\
				console.log(\"array is: \");\n\
				console.log(array[0]);\n\
				if(array.length > 1){\n\
				  	array.shift();\n\
					var request = new XMLHttpRequest();\n\
					request.onreadystatechange = function() { \n\
					if (this.readyState == 4) { \n\
						if (this.status == 200) { \n\
						if (this.responseText != null){ \n\
							document.getElementById(\"speedValue\").innerHTML = this.responseText; \n\
						}}}} \n\
					var url = \"/speedSlider=\" + array[0];\n\
					request.open(\"GET\", url, true);\n\
					request.send();\n\
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
								document.getElementById(\"speedSlider\").value = \"0\"; \n\
								document.getElementById(\"rangenumber\").value = \"0\"; \n\
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
				    let request = this.responseText; \n\
					if (request == \"eStop pressed\") {\n\
					document.getElementById(\"eBox\").innerHTML = \"E-STOP PRESSED!\";\n\
					document.getElementById(\"eBox\").className = \"Full Red\";\n\
					document.getElementById(\"startLabel\").innerHTML = \"Stopped\";\n\
					document.getElementById(\"outLabel\").innerHTML = \"\"; \n\
					document.getElementById(\"speedValue\").innerHTML = \"\"; \n\
					document.getElementById(\"speedSlider\").value = \"0\"; \n\
					document.getElementById(\"rangenumber\").value = \"0\"; \n\
					array[1] = 0;\n\
					} else {\n\
					document.getElementById(\"eBox\").innerHTML = \"Ready to run\";\n\
					document.getElementById(\"eBox\").className = \"Full Green\";\n\
					}\n\
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
				setTimeout(\"ReadReload()\", 500); }";
}

const char* HTMLGetSwitchAnalogData(){
	return " function GetSwitchAnalogData() { \n\
              var request = new XMLHttpRequest(); \n\
              request.onreadystatechange = function() {\n\
                if (this.readyState == 4) { \n\
				 if(this.status == 200) { \n\
                  if (this.responseText != null) { \n\
                    let recieved = this.responseText; \n\
					console.log(recieved);\n\
					let myArray = recieved.split(\"!\"); \n\
					document.getElementById(\"startBox\").innerHTML = myArray[0];\n\
					document.getElementById(\"startBox\").className = myArray[1];\n\
					document.getElementById(\"fwdRevBox\").innerHTML = myArray[2];\n\
					document.getElementById(\"fwdRevBox\").className = myArray[3];\n\
					document.getElementById(\"speedBox\").innerHTML = myArray[4];\n\
					document.getElementById(\"speedBox\").className = myArray[5];\n\
				}}}}\n\
              request.open(\"GET\", \"/ajax_switch\", true); \n\
			  request.send(null); }";
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
            .slider {\
              -webkit-appearance: none;\
              margin-left: 10px;\
              margin-right: 15px;\
              margin-top: 5px;\
              vertical-align: middle;\
              width: 210px; /* width */\
              height: 20px; /* Specified height */\
              background: #FAFAFA; /* Grey background */\
              border-radius: 5px;\
              background-size: 70% 100%;\
              background-repeat: no-repeat;\
            }\
            .slider::-moz-range-thumb {\
              -webkit-appearance:none;\
              width: 25px;\
              height: 25px;\
              border-radius: 50%;\
              background: #FF4500;\
              cursor: ew-resize;\
              box-shadow: 0 0 2px 0 #555;\
              transition: background .3s ease-in-out;\
            }\
            .slider::-moz-range-track{\
              -webkit-appearance: none;\
              box-shawdow: none;\
              border: none;\
              background: transparent;\
            }\
            .number {\
              vertical-align:bottom;\
              width:50px;\
              padding: 4px 10px;\
              border: 1px solid #bbb;\
              border-radius: 5px;\
            }\
";
}

const char* HTMLOutputFormat(){
	return "h1 {text-align:center;}\
			.centered {text-align:center; margin: 10px;\
				font-size: 30px;\
				color: white;\
			}\
			.control-panel{position:relative;\
				height:60%; width:80%;\
				text-align:center; margin:auto;\
				vertical-align:middle; min-width: 615px}\
			.start-position{margin-right: 5px; display:inline-block}\
			.stop-position{margin-left: 5px; display:inline-block}\
			.jogOut-position{margin: 5px 5px; display:inline-block}\
			.jogIn-position{margin: 5px 5px; display:inline-block}\
			.out-position{margin: 5px 5px; display:inline-block}\
			.in-position{margin: 5px 5px; display:inline-block}\
			div.box {\
                padding: 15px;\
                text-align: center;\
                font-size: 20px;\
				color: white;\
            }\
			.Red {background-color: #FC0303;}\
			.Green {background-color: #4CAF50;}\
			body{\
			background-color: LightBlue;\
			}\
			label.box {\
                text-align: center;\
                display: inline;\
                font-size: 30px;\
				color: white;\
                }\
			.Full {padding: 10px;}\
			.Middle {padding: 10px 0px 10px 0px;}\
			.Left {padding: 10px 0px 10px 10px;}\
			.Right {padding: 10px 10px 10px 0px;}\
			.Basic {background-color: black;}\
			.Yellow {background-color: orange;}\
			.Clear {border: 0px blue;}\
			.buttonBeside {\
			  width: 150px;\
              border: none;\
              color: white;\
			  padding: 15px 30px;\
              text-align: center;\
              text-decoration: none;\
              font-size: 25px;\
              cursor: pointer;\
            }";
}
const char* HTMLOutputBody(){
  return "<body onbeforeunload=\"return BeforeClose()\" onload=\"ReadReload()\"> \n\
  <h1>AGO Environmental Winch Controls</h1>\n\
  <div class=\"box\"><span class=\"Full Basic\">Mode: Computer Controls</span></div>\
  <div class=\"box\"><span class=\"Full Green\" id=\"eBox\" style=\"border-style: solid; border-color: black\">Ready to run</span></div>\
  <div class = \"control-panel\"><div class=\"start-position\"><button type=\"button\" class=\"button buttonGreen\" id=\"startButton\" onclick=\"StartPressed()\">Start</button></div>\n\
  <div class=\"stop-position\"><button type=\"button\" class=\"button buttonRed\" id=\"stopButton\" onclick=\"StopPressed()\">Stop</button></div>\n\
  <br><div class=\"jogOut-position\">\
  <button type=\"button\" class=\"buttonBeside buttonBasic\" id=\"jogOutButton\" onmousedown=\"JogOutPressed()\" onmouseup=\"JogOutReleased()\" onmouseleave=\"JogOutReleased()\">Jog Out</button>\n\
  </div>\
  <div class=\"out-position\"><button type=\"button\" class=\"button buttonBasic\" id=\"outButton\" onclick=\"OutPressed()\">&#160;Out&#160; </button></div>\n\
  <div class=\"in-position\"><button type=\"button\" class=\"button buttonBasic\" id=\"inButton\" onclick=\"InPressed()\">&#160;&#160;In&#160;&#160;</button></div>\n\
  <div class=\"jogIn-position\">\
  <button type=\"button\" class=\"buttonBeside buttonBasic\" id=\"jogInButton\" onmousedown=\"JogInPressed()\" onmouseup=\"JogInReleased()\" onmouseleave=\"JogInReleased()\">Jog In</button>\n\
  </div></div>\n\
  <div class = \"centered\">\n\
  <input type=\"range\" class=\"slider\" id=\"speedSlider\" min=\"0\" max=\"100\" value=\"0\" step=\"5\" oninput=\"rangenumber.value = value\" onchange=\"sliderPreProcessing()\"></input>\n\
  <input type=\"number\" class=\"number\" id=\"rangenumber\" min=\"0\" max=\"100\" step = \"5\" oninput=\"sliderPreProcessing()\"></input>\n\
  </div>\n\
  <br>";
}
const char* HTMLOutputBody1(){
  return "<div class = \"centered\">\n\
  <label for=\"startButton\" id = \"startLabel\" class=\"box Left Basic\">Stopped</label>\n\
  <label for=\"outButton\" id = \"outLabel\" class=\"box Middle Basic\"></label>\n\
  <label for=\"speedSlider\" id = \"speedValue\" class=\"box Right Basic\"></label>\n\
  </div>\n\
  <br>\
  <div class = \"centered\">\n\
  <label id=\"payoutM\" class=\"box Full Basic\">Cable Payout: </label>\n\
  <label id=\"payoutMeters\" class=\"box Full Basic\">100m</label>\n\
  <label id=\"payoutS\" class=\"box Full Basic\">Cable Speed: </label>\n\
  <label id=\"payoutSpeed\" class=\"box Full Basic\">3m/min</label>\n\
  </div>\
  ";
}

const char* HTMLInputHeader(){
	return "<title>Manual Winch Mode</title>\
            	<style>\
                span.box {\
                padding: 15px 15px;\
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
				}\
              </style>";
}

const char* HTMLInputBody(){
  return "  <h1>AGO Environmental Winch Monitoring System</h1>\
			<p><span class=\"box Basic\">Mode: Manual Monitoring</span></p><br></br>\
			<p><span class=\"box Red\" id=\"startBox\">Stopped</span></p><br></br>\
			<p><span class=\"box Clear\" id=\"fwdRevBox\"></span></p><br></br>\
			<p><span class=\"box Clear\" id=\"speedBox\"></span></p>\
			";
}