import subprocess
import os

# Function to display banner
def display_banner():
    print("""
                        ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀                      \033[91m              ⠀    ⢀⠀⠀⠀⠀\033[0m
          ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀                                         \033[91m             ⣠⠏⠀⠀⠀⣠⡇⠀⠀⠀⠀\033[0m
          ██████╗ ███████╗ █████╗ ██████╗        ██████╗██████╗ \033[91m        ⠀⠀⠀⢠⣾⠏⠀⠀⠀⣰⠏⠀⠀⢀⠄⠀\033[0m
          ██╔══██╗██╔════╝██╔══██╗██╔══██╗      ██╔════╝╚════██╗\033[91m        ⠀⢀⣴⡿⠃⠀⠀⢀⣾⠏⠀⠀⣰⠏⠀⠀\033[0m
          ██████╔╝█████╗  ███████║██████╔╝█████╗██║      █████╔╝\033[91m        ⣠⣾⠟⠀⠀⢀⣰⣿⠃⠀⠀⣴⠏⠀⠀⣰\033[0m
          ██╔══██╗██╔══╝  ██╔══██║██╔══██╗╚════╝██║     ██╔═══╝ \033[91m      ⢀⣼⡿⠋⠀⠀⣰⣾⠏⠀⠀⣠⡾⠃⠀⢀⣾⠁\033[0m
          ██████╔╝███████╗██║  ██║██║  ██║      ╚██████╗███████╗\033[91m     ⣠⣿⠛⠀⠀⢠⣾⡿⠃⠀⢀⣴⡿⠃⠀⣠⡾⠃⠀\033[0m
          ╚═════╝ ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝       ╚═════╝╚══════╝\033[91m   ⢠⣾⠟⠁⠀⢀⣴⡿⠛⠀⠀⣰⣾⠟⠀⢀⣴⡟⠀⠀⠀\033[0m
                                                                \033[91m ⢀⣴⠛⠁⠀⠀⣴⡿⠋⠀⠀⣠⣾⡿⠁⠀⣠⣿⠋⠀⠀⠀⠀\033[0m
                                                                \033[91m⠴⠛⠁⠀⠀⢀⡾⠋⠁⠀⣠⣾⠟⠁⠀⣠⣾⡿⠃⠀⠀⠀⠀⠀\033[0m
⠀⠀⠀                                                             \033[91m    ⠖⠋⠀⠀⢀⣴⠟⠁⠀⢠⣾⠟⠁⠀⠀⠀⠀⠀⠀⠀\033[0m
⠀⠀⠀⠀⠀⠀⠀                                                         \033[91m       ⠔⠋⠀⠀⢀⡴⠋⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀\033[0m
        ⠀⠀⠀⠀⠀⠀⠀⠀                                                \033[91m    ⠀⠀     ⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\033[0m
""")

# Function to run selected cloud storage script
def run_script(choice):
    try:
        if choice == 1:
            subprocess.run(["Profiles/OneDrive.o"], check=True)
        elif choice == 2:
            subprocess.run(["Profiles/GoogleDrive.o"], check=True)
        elif choice == 3:
            subprocess.run(["Profiles/Dropbox.o"], check=True)
        elif choice == 4:
            subprocess.run(["Profiles/Discord.o"], check=True)
        elif choice == 5:
            subprocess.run(["Profiles/Backdoor-C2.o"], check=True)
        elif choice == 6:
            subprocess.run(["Profiles/XOR.o"], check=True)
        elif choice == 7:
            subprocess.run(["Profiles/RSA.o"], check=True)
        elif choice == 8:
            subprocess.run(["Profiles/DES.o"], check=True)
        elif choice == 9:
            print("\033[91m waiting you for the next attack, Goodbye!\033[0m")
            return True
        else:
            print("Invalid choice. Please select a number between 1 and 9")
    except subprocess.CalledProcessError as e:
        print(f"Error occurred while running the script: {e}")
    except PermissionError:
        print("Permission denied. Please check the file permissions.")
    return False

# Main function
def main():
    while True:
        display_banner()
        print("+---------------------------------------------------------------------------------------------------+")
        print("| \033[93mProfiles                       Encryptions                         Actors                Country  \033[0m|")
        print("+---------------------------------------------------------------------------------------------------+")
        print("| 1. OneDrive-API       |        AES encryption         |          Fancy Bear         |      RUSSIA |")
        print("| 2. GoogleDrive-API    |        RC4 encryption         |          Gossamer Bear      |      RUSSIA |")
        print("| 3. Dropbox-API        |        AES encryption         |          Cozy Bear          |      RUSSIA |")
        print("| 4. Discord-Bot        |       ChaCha encryption       |          Ember Bear         |      RUSSIA |")
        print("| 5. Backdoor-Listener  |        TLS encryption         |          Venomous Bear      |      RUSSIA |")
        print("| 6. XOR-backend        |        XOR encryption         |          Energetic Bear     |      RUSSIA |")
        print("| 7. Private Key-2048   |        RSA encryption         |          Voodoo Bear        |      RUSSIA |")
        print("| 8. DES-8-bytes        |        DES encryption         |          Primitive Bear     |      RUSSIA |")
        print("+---------------------------------------------------------------------------------------------------+")
        print("| 9. Exit                                                                                           |")
        print("+---------------------------------------------------------------------------------------------------+")
        try:
            choice = int(input("\n[\033[93m*\033[0m] \033[91mBEAR >> \033[0m"))
            if run_script(choice):
                break
        except ValueError:
            print("Invalid input. Please enter a number between 1 and 9.")
        except KeyboardInterrupt:
            print("\033[91m waiting you for the next attack, Goodbye!\033[0m")
            os._exit(0)

if __name__ == "__main__":
    main()
