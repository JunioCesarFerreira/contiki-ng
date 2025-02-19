import paramiko

def fetch_file_without_blanks(
    host: str,
    port: int,
    username: str,
    password: str,
    remote_path: str,
    local_path: str
) -> None:
    """
    Connects via SSH to a remote host and uses 'cat' to read the contents of an XML file.
    Removes blank lines from the content before saving it locally.
    
    :param host: IP address or hostname of the server/VM
    :param port: SSH port (e.g., 22 or 2222 if you're using port forwarding)
    :param username: Username for authentication
    :param password: User's password
    :param remote_path: Full path to the XML file on the server (e.g., /home/user/file.xml)
    :param local_path: Local path to save the file (e.g., ./my_file.xml)
    """
    
    # Create an SSHClient object
    ssh_client = paramiko.SSHClient()
    
    # Define the policy to accept unknown host keys
    ssh_client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    
    try:
        # Connect to the server/VM via SSH
        ssh_client.connect(hostname=host, port=port, username=username, password=password)
        
        # Execute the 'cat' command to read the remote file
        command = f"cat {remote_path}"
        stdin, stdout, stderr = ssh_client.exec_command(command)
        
        # Read the command's output
        output = stdout.read().decode('utf-8', errors='replace')
        error_output = stderr.read().decode('utf-8', errors='replace')
        
        if error_output.strip():
            print(f"[ERROR] While executing the command: {error_output}")
        else:
            print("[INFO] Successfully retrieved the XML file content.")
            
            # Remove blank lines
            lines_without_blanks = [
                line for line in output.splitlines() 
                if line.strip()  # only keep lines that aren't empty
            ]
            final_content = "\n".join(lines_without_blanks)
            
            # Save the content to the local file (without blank lines)
            with open(local_path, 'w', encoding='utf-8') as f:
                f.write(final_content)
            
            print(f"[INFO] File saved to: {local_path}")
    
    except Exception as e:
        print(f"[ERROR] SSH connection or command failed: {str(e)}")
    finally:
        # Close the SSH connection
        ssh_client.close()

    
# parmeters usage
HOST = "172.26.96.1"       # If using NAT + port forwarding
PORT = 2222                # SSH port or forwarded port (e.g., 22 if using Bridged)
USERNAME = "ubuntu"        # VM user
PASSWORD = "ubuntu"        # User's password
REMOTE_PATH = "/home/ubuntu/Desktop/Simulations"

REMOTE_FILES = ["/simulation.csc", "/positions.dat"]

LOCAL_PATHS = ["./simulation_from_vm.xml", "./positions_from_vm.dat"]

for i in range(0, 2):
    fetch_file_without_blanks(HOST, PORT, USERNAME, PASSWORD, 
                                  REMOTE_PATH+REMOTE_FILES[i], LOCAL_PATHS[i])
