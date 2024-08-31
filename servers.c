#include <string.h>
#include "servers.h"
#include "asc.h"
#include "errors.h"
#include "misc.h"
#include "multiplayer.h"
#include "io/elpathwrapper.h"
#define MAX_SERVERS 10
typedef struct {
	char id[20];                                            // The ID of the server - to be specified on the command line
	char dir[20];                                           // The dir under $CONF_DIR
	unsigned char address[60];
	int port;
	char desc[100];                                         // Description of the server - to be shown on in the Server Selection screen
} server_def;
server_def servers[MAX_SERVERS];                // The details of all the servers we know about
int num_servers = 0;
int cur_server = -1;
char *check_server_id_on_command_line();        // From main.c
const char *get_server_name(void) {
	if (cur_server >= 0) {
		return servers[cur_server].id;
	} else {
		return "<unset>";
	}
}
int find_server_from_id(const char *id) {
	int i;
	if (num_servers <= 0) {
		return -1;
	}
	for (i = 0; i < num_servers; i++) {
		if (!strcasecmp(servers[i].id, id)) {
			return i;
		}
	}
	return -1;
}
void set_server_details() {
	char id[20];
	int num;
	safe_strncpy(id, check_server_id_on_command_line(), sizeof(id));
	if (!strcmp(id, "")) {
		safe_strncpy(id, "main", sizeof(id));
	}
	num = find_server_from_id(id);
	if (num == -1) {
		// Oops... what they they specify on the command line?
		LOG_ERROR("Server profile not found in servers.lst for server: %s. Failover to server: main.", id);
		// Failover to the main server
		num = find_server_from_id("main");
		if (num == -1) {
			// Error, this is a problem!
			LOG_ERROR("Fatal error: Server profile not found in servers.lst for server: main");
			exit(1);
		}
	}
	// We found a valid profile so set some vars
	LOG_DEBUG("Utilisation du profil pour le serveur : %s", servers[num].id);
	cur_server = num;
	safe_strncpy((char *)server_address, (char *)servers[num].address, sizeof(server_address));
	port = servers[num].port;
	// Check if the config directory for the profile exists and if not then create and
	// copy main's ini file into it
	if (!check_configdir()) {
		char src[1000];
		char dest[1000];
		mkdir_tree(get_path_config(), 0);
		// First, try to copy the ini file out of $CONF/main
		safe_snprintf(src, sizeof(src), "%smain/le.ini", get_path_config_base());
		safe_snprintf(dest, sizeof(dest), "%sle.ini", get_path_config());
		copy_file(src, dest);
		// Secondly, try to copy the ini file out of $CONF (this will fail without harm if above succeeds)
		safe_snprintf(src, sizeof(src), "%s/le.ini", get_path_config_base());
		safe_snprintf(dest, sizeof(dest), "%sle.ini", get_path_config());
		copy_file(src, dest);
	}
	/**
	 * en copiant le fichier servers.lst à la racine du dossier perso,
	 * il pourra être chargé quelque soit l'emplacement du jeu
	 * (vu qu'il est ouvert avant que le fichier le.ini ne soit chargé,
	 * donc sans avoir connaissance d'un data_dir modifié)
	 */
	if (file_exists(get_path_config_base())) {
		char dest[1000];
		safe_snprintf(dest, sizeof(dest), "%sservers.lst", get_path_config_base());
		if (file_exists("servers.lst") && !file_exists(dest)) {
			copy_file("servers.lst", dest);
		}
	}
}
const char *get_server_dir() {
	if (cur_server >= 0) {
		return servers[cur_server].dir;
	} else {
		return "";
	}
}
void load_server_list(const char *filename) {
	int f_size;
	FILE *f = NULL;
	char *server_list_mem;
	int istart, iend, i, section;
	char string[128];
	int len;
	f = open_file_config(filename, "rb");
	if (f == NULL) {
		// Error, this is a problem!
		const char *err_message = "Fatal error: %s file missing!\n";
		LOG_ERROR(err_message, filename);
		fprintf(stderr, err_message, filename);
		exit(1);
	}
	// Ok, allocate memory for it and read it in
	fseek(f, 0, SEEK_END);
	f_size = ftell(f);
	if (f_size <= 0) {
		const char *err_message = "Fatal error: %s is empty!\n";
		LOG_ERROR(err_message, filename);
		fprintf(stderr, err_message, filename);
		fclose(f);
		exit(1);
	}
	server_list_mem = (char *)calloc(f_size, 1);
	fseek(f, 0, SEEK_SET);
	if (fread(server_list_mem, 1, f_size, f) != f_size) {
		const char *err_message = "Fatal error: %s read failed!\n";
		LOG_ERROR(err_message, filename);
		fprintf(stderr, err_message, filename);
		free(server_list_mem);
		fclose(f);
		exit(1);
	}
	fclose(f);
	istart = 0;
	num_servers = 0;
	while (istart < f_size) {
		// Find end of the line
		for (iend = istart; iend < f_size; iend++) {
			if (server_list_mem[iend] == '\n' || server_list_mem[iend] == '\r') {
				break;
			}
		}
		// Parse this line
		if (iend > istart) {
			section = 0;
			len = 0;
			for (i = istart; i < iend; i++) {
				if (server_list_mem[i] == '#') {
					break;  // This is a comment so ignore the rest of the line
				} else if (section < 4 && (server_list_mem[i] == ' ' || server_list_mem[i] == '\t' || i == iend)) {
					if (num_servers >= MAX_SERVERS) {
						const char *errstg = "Fatal error: Too many servers specified in";
						LOG_ERROR("%s %s", errstg, filename);
						fprintf(stderr, "%s %s\n", errstg, filename);
						exit(1);
					}
					// This is the end of a section so store it (except the description)
					// as we include whitespace in the description
					string[len] = '\0';
					switch (section) {
					case 0:                 // Server ID
						safe_strncpy(servers[num_servers].id, string, sizeof(servers[num_servers].id));
						break;
					case 1:                 // Config dir
						safe_strncpy(servers[num_servers].dir, string, sizeof(servers[num_servers].dir));
						break;
					case 2:                 // Server address
						safe_strncpy((char *)servers[num_servers].address, string, sizeof(servers[num_servers].address));
						break;
					case 3:                 // Server port
						servers[num_servers].port = atoi(string);
						break;
					}
					section++;
					// Reset the length to start the string again
					len = 0;
					// Skip any more spaces
					while (i < iend) {
						if (server_list_mem[i + 1] != ' ' && server_list_mem[i + 1] != '\t') {
							break;
						}
						i++;
					}
				} else { //if (server_list_mem[i] == ) // Valid char!!)
					string[len] = server_list_mem[i];
					len++;
				}
			}
			if (i > istart) {
				// Anything left should be the description so store it now
				string[len] = '\0';
				safe_strncpy(servers[num_servers].desc, string, sizeof(servers[num_servers].desc));
				// Check the line was valid
				if (!strcmp(servers[num_servers].id, "") || !strcmp(servers[num_servers].dir, "") || !strcmp((char *)servers[num_servers].address, "") || servers[num_servers].port == 0 || !strcmp(servers[num_servers].desc, "")) {
					LOG_ERROR("%s: Invalid server details specified in %s - (%d) %s", "Servers list error", filename, num_servers, servers[num_servers].id);
					break;          // Bail, but do the free first
				}
				// we added a valid line
				num_servers++;
			}
		}
		// Move to next line
		istart = iend + 1;
	}
	free(server_list_mem);
}
