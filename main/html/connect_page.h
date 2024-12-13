static const char connect_html[] = R"rawliteral(
<!DOCTYPE html>
<html lang='en'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title> Connect to WiFi </title>
    <link rel='stylesheet' type='text/css' href='/style.css'>
</head>

<body>
    <div class='container'>
        <h2>WiFi router connection</h2>
        <form action='/validate_connect' method='post'>
            <label>SSID: </label>
            <input type='text' placeholder='Enter SSID' name='ssid' required>

            <label>Password: </label>
            <input type='password' placeholder='Enter Password' name='password' required>

            <button type='submit'>Connect</button>
        </form>
    </div>
</body>
</html>
)rawliteral";
