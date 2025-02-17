import os
import paramiko
from scp import SCPClient

# Configurações
local_directory = './rpl-udp-client-webserver'
remote_directory = '/home/ubuntu/contiki-ng/examples/rpl-udp-client-webserver'
vm_hostname = '172.26.96.1'
vm_username = 'ubuntu'
vm_password = 'ubuntu'  
vm_port = 2222

# Função para criar conexão SSH
def create_ssh_client(hostname, port, username, password):
    client = paramiko.SSHClient()
    client.load_system_host_keys()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(hostname, port=port, username=username, password=password)
    return client

# Função para enviar arquivos via SCP
def send_files_scp(client, local_path, remote_path):
    with SCPClient(client.get_transport()) as scp:
        for root, dirs, files in os.walk(local_path):
            for file in files:
                local_file_path = root.replace("\\", "/") + "/" + file
                remote_file_path = remote_path + "/" + file
                if "/webserver" in local_file_path:
                    remote_file_path = remote_path + "/webserver/" + file
                print(f"Sending {local_file_path} to {remote_file_path}")
                scp.put(local_file_path, remote_file_path)

# Execução principal
if __name__ == "__main__":
    ssh = create_ssh_client(vm_hostname, vm_port, vm_username, vm_password)
    send_files_scp(ssh, local_directory, remote_directory)
    ssh.close()
    print("Transfer completed.")