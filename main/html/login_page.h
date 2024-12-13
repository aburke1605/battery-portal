static const char login_html[] = R"rawliteral(
<!DOCTYPE html>
<html lang='en'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>AP Login Form</title>
    <link rel='stylesheet' type='text/css' href='/style.css'>
</head>

<body>
    <div class='container'>
        <h2>AP Login Form</h2>
        <form action='/validate_login' method='post'>
            <label for='username'>Username</label>
            <input type='text' id='username' name='username' required oninvalid="this.setCustomValidity('Please input your username.')" oninput="setCustomValidity('')">

            <label for='password'>Password</label>
            <input type='password' id='password' name='password' required oninvalid="this.setCustomValidity('Please input your password.')" oninput="setCustomValidity('')">

            <div class='actions'>
                <label> <input type='checkbox'> Remember </label>
                <a href='#'>Forgot password?</a>
            </div>

            <button type='submit'>Get started</button>
        </form>
        <img src='/image/aceon.png' alt='Company Logo'>
    </div>
</body>
</html>
)rawliteral";
