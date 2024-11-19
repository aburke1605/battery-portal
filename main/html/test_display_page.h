static const char display_html[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Battery & LED Control</title>
    <style>
      *,::after,::before{--tw-border-spacing-x:0;--tw-border-spacing-y:0;--tw-translate-x:0;--tw-translate-y:0;--tw-rotate:0;--tw-skew-x:0;--tw-skew-y:0;--tw-scale-x:1;--tw-scale-y:1;--tw-pan-x: ;--tw-pan-y: ;--tw-pinch-zoom: ;--tw-scroll-snap-strictness:proximity;--tw-gradient-from-position: ;--tw-gradient-via-position: ;--tw-gradient-to-position: ;--tw-ordinal: ;--tw-slashed-zero: ;--tw-numeric-figure: ;--tw-numeric-spacing: ;--tw-numeric-fraction: ;--tw-ring-inset: ;--tw-ring-offset-width:0px;--tw-ring-offset-color:#fff;--tw-ring-color:rgb(59 130 246 / 0.5);--tw-ring-offset-shadow:0 0 #0000;--tw-ring-shadow:0 0 #0000;--tw-shadow:0 0 #0000;--tw-shadow-colored:0 0 #0000;--tw-blur: ;--tw-brightness: ;--tw-contrast: ;--tw-grayscale: ;--tw-hue-rotate: ;--tw-invert: ;--tw-saturate: ;--tw-sepia: ;--tw-drop-shadow: ;--tw-backdrop-blur: ;--tw-backdrop-brightness: ;--tw-backdrop-contrast: ;--tw-backdrop-grayscale: ;--tw-backdrop-hue-rotate: ;--tw-backdrop-invert: ;--tw-backdrop-opacity: ;--tw-backdrop-saturate: ;--tw-backdrop-sepia: ;--tw-contain-size: ;--tw-contain-layout: ;--tw-contain-paint: ;--tw-contain-style: }::backdrop{--tw-border-spacing-x:0;--tw-border-spacing-y:0;--tw-translate-x:0;--tw-translate-y:0;--tw-rotate:0;--tw-skew-x:0;--tw-skew-y:0;--tw-scale-x:1;--tw-scale-y:1;--tw-pan-x: ;--tw-pan-y: ;--tw-pinch-zoom: ;--tw-scroll-snap-strictness:proximity;--tw-gradient-from-position: ;--tw-gradient-via-position: ;--tw-gradient-to-position: ;--tw-ordinal: ;--tw-slashed-zero: ;--tw-numeric-figure: ;--tw-numeric-spacing: ;--tw-numeric-fraction: ;--tw-ring-inset: ;--tw-ring-offset-width:0px;--tw-ring-offset-color:#fff;--tw-ring-color:rgb(59 130 246 / 0.5);--tw-ring-offset-shadow:0 0 #0000;--tw-ring-shadow:0 0 #0000;--tw-shadow:0 0 #0000;--tw-shadow-colored:0 0 #0000;--tw-blur: ;--tw-brightness: ;--tw-contrast: ;--tw-grayscale: ;--tw-hue-rotate: ;--tw-invert: ;--tw-saturate: ;--tw-sepia: ;--tw-drop-shadow: ;--tw-backdrop-blur: ;--tw-backdrop-brightness: ;--tw-backdrop-contrast: ;--tw-backdrop-grayscale: ;--tw-backdrop-hue-rotate: ;--tw-backdrop-invert: ;--tw-backdrop-opacity: ;--tw-backdrop-saturate: ;--tw-backdrop-sepia: ;--tw-contain-size: ;--tw-contain-layout: ;--tw-contain-paint: ;--tw-contain-style: }*,::after,::before{box-sizing:border-box;border-width:0;border-style:solid;border-color:#e5e7eb}::after,::before{--tw-content:''}:host,html{line-height:1.5;-webkit-text-size-adjust:100%;-moz-tab-size:4;tab-size:4;font-family:ui-sans-serif,system-ui,sans-serif,"Apple Color Emoji","Segoe UI Emoji","Segoe UI Symbol","Noto Color Emoji";font-feature-settings:normal;font-variation-settings:normal;-webkit-tap-highlight-color:transparent}body{margin:0;line-height:inherit}hr{height:0;color:inherit;border-top-width:1px}abbr:where([title]){text-decoration:underline dotted}h1,h2,h3,h4,h5,h6{font-size:inherit;font-weight:inherit}a{color:inherit;text-decoration:inherit}b,strong{font-weight:bolder}code,kbd,pre,samp{font-family:ui-monospace,SFMono-Regular,Menlo,Monaco,Consolas,"Liberation Mono","Courier New",monospace;font-feature-settings:normal;font-variation-settings:normal;font-size:1em}small{font-size:80%}sub,sup{font-size:75%;line-height:0;position:relative;vertical-align:baseline}sub{bottom:-.25em}sup{top:-.5em}table{text-indent:0;border-color:inherit;border-collapse:collapse}button,input,optgroup,select,textarea{font-family:inherit;font-feature-settings:inherit;font-variation-settings:inherit;font-size:100%;font-weight:inherit;line-height:inherit;letter-spacing:inherit;color:inherit;margin:0;padding:0}button,select{text-transform:none}button,input:where([type=button]),input:where([type=reset]),input:where([type=submit]){-webkit-appearance:button;background-color:transparent;background-image:none}:-moz-focusring{outline:auto}:-moz-ui-invalid{box-shadow:none}progress{vertical-align:baseline}::-webkit-inner-spin-button,::-webkit-outer-spin-button{height:auto}[type=search]{-webkit-appearance:textfield;outline-offset:-2px}::-webkit-search-decoration{-webkit-appearance:none}::-webkit-file-upload-button{-webkit-appearance:button;font:inherit}summary{display:list-item}blockquote,dd,dl,figure,h1,h2,h3,h4,h5,h6,hr,p,pre{margin:0}fieldset,menu,ol,ul{margin:0;padding:0}dialog,legend{padding:0}menu,ol,ul{list-style:none}textarea{resize:vertical}input::placeholder,textarea::placeholder{opacity:1;color:#9ca3af}[role=button],button{cursor:pointer}:disabled{cursor:default}audio,canvas,embed,iframe,img,object,svg,video{display:block;vertical-align:middle}img,video{max-width:100%;height:auto}[hidden]:where(:not([hidden=until-found])){display:none}.mx-auto{margin-left:auto;margin-right:auto}.mb-6{margin-bottom:1.5rem}.mt-10{margin-top:2.5rem}.flex{display:flex}.h-16{height:4rem}.w-full{width:100%}.max-w-4xl{max-width:56rem}.flex-col{flex-direction:column}.items-center{align-items:center}.justify-between{justify-content:space-between}.gap-4{gap:1rem}.space-y-10>:not([hidden])~:not([hidden]){--tw-space-y-reverse:0;margin-top:calc(2.5rem*calc(1 - var(--tw-space-y-reverse)));margin-bottom:calc(2.5rem*var(--tw-space-y-reverse))}.space-y-4>:not([hidden])~:not([hidden]){--tw-space-y-reverse:0;margin-top:calc(1rem*calc(1 - var(--tw-space-y-reverse)));margin-bottom:calc(1rem*var(--tw-space-y-reverse))}.rounded-lg{border-radius:.5rem}.bg-blue-100{--tw-bg-opacity:1;background-color:rgb(219 234 254/var(--tw-bg-opacity, 1))}.bg-green-500{--tw-bg-opacity:1;background-color:rgb(34 197 94/var(--tw-bg-opacity, 1))}.bg-green-600,.hover\:bg-green-600:hover{--tw-bg-opacity:1;background-color:rgb(22 163 74/var(--tw-bg-opacity, 1))}.bg-white{--tw-bg-opacity:1;background-color:rgb(255 255 255/var(--tw-bg-opacity, 1))}.p-8{padding:2rem}.py-3{padding-top:.75rem;padding-bottom:.75rem}.text-center{text-align:center}.font-sans{font-family:ui-sans-serif,system-ui,sans-serif,"Apple Color Emoji","Segoe UI Emoji","Segoe UI Symbol","Noto Color Emoji"}.text-4xl{font-size:2.25rem;line-height:2.5rem}.text-lg{font-size:1.125rem;line-height:1.75rem}.font-bold{font-weight:700}.font-semibold{font-weight:600}.text-blue-700{--tw-text-opacity:1;color:rgb(29 78 216/var(--tw-text-opacity, 1))}.hover\:text-blue-900:hover,.text-blue-900{--tw-text-opacity:1;color:rgb(30 58 138/var(--tw-text-opacity, 1))}.text-gray-700{--tw-text-opacity:1;color:rgb(55 65 81/var(--tw-text-opacity, 1))}.text-gray-800{--tw-text-opacity:1;color:rgb(31 41 55/var(--tw-text-opacity, 1))}.text-white{--tw-text-opacity:1;color:rgb(255 255 255/var(--tw-text-opacity, 1))}.underline{text-decoration-line:underline}.shadow-lg,.shadow-md{box-shadow:var(--tw-ring-offset-shadow, 0 0 #0000),var(--tw-ring-shadow, 0 0 #0000),var(--tw-shadow)}.shadow-lg{--tw-shadow:0 10px 15px -3px rgb(0 0 0 / 0.1), 0 4px 6px -4px rgb(0 0 0 / 0.1);--tw-shadow-colored:0 10px 15px -3px var(--tw-shadow-color), 0 4px 6px -4px var(--tw-shadow-color)}.shadow-md{--tw-shadow:0 4px 6px -1px rgb(0 0 0 / 0.1), 0 2px 4px -2px rgb(0 0 0 / 0.1);--tw-shadow-colored:0 4px 6px -1px var(--tw-shadow-color), 0 2px 4px -2px var(--tw-shadow-color)}.outline-none{outline:2px solid transparent;outline-offset:2px}.ring-2{--tw-ring-offset-shadow:var(--tw-ring-inset) 0 0 0 var(--tw-ring-offset-width) var(--tw-ring-offset-color);--tw-ring-shadow:var(--tw-ring-inset) 0 0 0 calc(2px + var(--tw-ring-offset-width)) var(--tw-ring-color);box-shadow:var(--tw-ring-offset-shadow),var(--tw-ring-shadow),var(--tw-shadow, 0 0 #0000)}.ring-green-400{--tw-ring-opacity:1;--tw-ring-color:rgb(74 222 128 / var(--tw-ring-opacity, 1))}.focus\:outline-none:focus{outline:2px solid transparent;outline-offset:2px}.focus\:ring-2:focus{--tw-ring-offset-shadow:var(--tw-ring-inset) 0 0 0 var(--tw-ring-offset-width) var(--tw-ring-offset-color);--tw-ring-shadow:var(--tw-ring-inset) 0 0 0 calc(2px + var(--tw-ring-offset-width)) var(--tw-ring-color);box-shadow:var(--tw-ring-offset-shadow),var(--tw-ring-shadow),var(--tw-shadow, 0 0 #0000)}.focus\:ring-green-400:focus{--tw-ring-opacity:1;--tw-ring-color:rgb(74 222 128 / var(--tw-ring-opacity, 1))}
      </style>
</head>
<body class="bg-blue-100 font-sans">
    <!-- Container for the whole page -->
    <div class="max-w-4xl mx-auto mt-10 space-y-10">
        <!-- Battery Information Section -->
        <div class="bg-white p-8 rounded-lg shadow-lg">
            <h1 class="text-4xl font-bold text-center mb-6 text-gray-800">Battery Information</h1>
            <div class="flex flex-col gap-4 text-lg text-gray-700">
                <div class="flex justify-between items-center">
                    <span>State of charge:</span>
                    <span><span id='charge'></span> %</span>
                </div>
                <div class="flex justify-between items-center">
                    <span>Voltage:</span>
                    <span><span id='voltage'></span> V</span>
                </div>
                <div class="flex justify-between items-center">
                    <span>Current:</span>
                    <span> <span id='current'></span> A</span>
                </div>
                <div class="flex justify-between items-center">
                    <span>Temperature:</span>
                    <span><span id='temperature'></span> °C</span>
                </div>
            </div>
        </div>

        <!-- ESP32 LED Control Section -->
        <div class="bg-white p-8 rounded-lg shadow-lg">
            <h2 class="text-4xl font-bold text-center mb-6 text-gray-800">ESP32 LED Control</h2>
            <div class="flex flex-col space-y-4">
                <button class="w-full bg-green-500 text-white font-semibold py-3 rounded-lg shadow-md hover:bg-green-600 focus:outline-none focus:ring-2 focus:ring-green-400" onclick="fetch('/toggle').then(() => alert('LED Toggled'))">Toggle LED</button>
            </div>
        </div>
         <div class="bg-white p-8 rounded-lg shadow-lg">
          <div class="flex justify-between items-center">
              <img src="/image/aceon2.png" alt="Company Logo" class="h-16">
              <div class="flex flex-col space-y-4">
                  <a href="/about" class="text-blue-700 underline hover:text-blue-900">About AceOn</a>
                  <a href="/device" class="text-blue-700 underline hover:text-blue-900">Unit Layout</a>
              </div>
          </div>
      </div>
    </div>
</body>
<script>
    document.addEventListener('DOMContentLoaded', function() {
      let socket = new WebSocket('ws://' + location.host + '/ws');
      socket.onopen = function() {
          console.log('WebSocket connection established');
      };
      socket.onmessage = function(event) {
          try {
              let data = JSON.parse(event.data);
              document.getElementById('charge').innerHTML = data.charge;
              document.getElementById('voltage').innerHTML = data.voltage.toFixed(2);
              document.getElementById('current').innerHTML = data.current.toFixed(2);
              document.getElementById('temperature').innerHTML = data.temperature.toFixed(2);
          } catch (error) {
              console.error('Error parsing JSON:', error);
          }
      };
      socket.onerror = function(error) {
          console.error('WebSocket error:', error);
      };
      socket.onclose = function() {
          console.log('WebSocket connection closed');
      };
    })
</script>
</html>
)rawliteral";
