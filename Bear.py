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
            subprocess.run(["python3", "Profiles/general-beacon.py"], check=True)
        if choice == 2:
            subprocess.run(["python3", "Profiles/OneDrive.py"], check=True)
        elif choice == 3:
            subprocess.run(["python3", "Profiles/Dropbox.py"], check=True)
        elif choice == 4:
            subprocess.run(["php", "Profiles/XOR.php"], check=True)
        elif choice == 5:
            subprocess.run(["perl", "Profiles/DES.pl"], check=True)
        elif choice == 6:
            subprocess.run(["python3", "Profiles/Backdoor-C2.py"], check=True)
        elif choice == 7:
            subprocess.run(["python3", "Profiles/GoogleDrive.py"], check=True)
        elif choice == 8:
            subprocess.run(["php", "Profiles/RSA.php"], check=True)   
        elif choice == 9:
            subprocess.run(["python3", "Profiles/Discord.py"], check=True) 
        elif choice == 10:
            print("\033[91m waiting you for the next attack, Goodbye!\033[0m")
            return True
        else:
            print("Invalid choice. Please select a number between 1 and 10")
    except subprocess.CalledProcessError as e:
        print(f"Error occurred while running the script: {e}")
    return False

# Main function
def main():
    while True:
        display_banner()
        print("+---------------------------------------------------------------------------------------------------+")
        print("| 1. General Beacon                                                                                 |")
        print("+---------------------------------------------------------------------------------------------------+")
        print("| \033[93mProfiles                       Encryptions                         Actors                Country  \033[0m|")
        print("+---------------------------------------------------------------------------------------------------+")
        print("| 2. OneDrive-API       |        AES encryption         |          Fancy Bear         |      RUSSIA |")
        print("| 3. Dropbox-API        |        AES encryption         |          Cozy Bear          |      RUSSIA |")
        print("| 4. PHP-backend        |        XOR encryption         |          Energetic Bear     |      RUSSIA |")
        print("| 5. Perl 8-bytes       |        DES encryption         |          Primitive Bear     |      RUSSIA |")
        print("| 6. Backdoor-Listener  |        TLS encryption         |          Venomous Bear      |      RUSSIA |")
        print("| 7. GoogleDrive-API    |        RC4 encryption         |          Gossamer Bear      |      RUSSIA |")
        print("| 8. Private Key-2048   |        RSA encryption         |          Voodoo Bear        |      RUSSIA |")
        print("| 9. Discord-Bot        |       ChaCha encryption       |          Ember Bear         |      RUSSIA |")
        print("+---------------------------------------------------------------------------------------------------+")
        print("|10. Exit                                                                                           |")
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
