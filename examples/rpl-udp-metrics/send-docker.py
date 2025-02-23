import paramiko
from scp import SCPClient

def create_ssh_client(hostname, port, username, password):
    client = paramiko.SSHClient()
    client.load_system_host_keys()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(hostname, port=port, username=username, password=password)
    return client

def send_files_scp(client, local_path, remote_path, source_files, target_files):
    if len(source_files) != len(target_files):
        print("The number of source files is not equal number of targets.")
        return
    with SCPClient(client.get_transport()) as scp:
        for src, dest in zip(source_files, target_files):
            local_file_path = local_path + "/" + src
            remote_file_path = remote_path + "/" + dest
            print(f"Sending {local_file_path} to {remote_file_path}")
            scp.put(local_file_path, remote_file_path)  

# I chose to use fixed '/' instead of `os.path.join` because destiny is linux.
LOCAL_DIRECTORY = '.'
REMOTE_DIRECTORY = '/opt/contiki-ng/tools/cooja'
CONTAINER_HOSTNAME = 'localhost'
CONTAINER_USERNAME = 'root'
CONTAINER_PASSWORD = 'root'  
CONTAINER_PORT = 2223

LOCAL_FILES = [
    "simulation.xml", 
    "positions.dat",
    "Makefile", 
    "project-conf.h",
    "udp-client.c", 
    "udp-server.c"]

REMOTE_FILES = [
    "simulation.csc", 
    "positions.dat",
    "Makefile", 
    "project-conf.h",
    "udp-client.c", 
    "udp-server.c"]

ssh = create_ssh_client(CONTAINER_HOSTNAME, CONTAINER_PORT, CONTAINER_USERNAME, CONTAINER_PASSWORD)
send_files_scp(ssh, LOCAL_DIRECTORY, REMOTE_DIRECTORY, LOCAL_FILES, REMOTE_FILES)
ssh.close()
print("Transfer completed.")