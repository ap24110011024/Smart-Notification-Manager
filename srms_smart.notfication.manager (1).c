                      //SOURCE CODE
                      
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>   

#define CREDENTIAL_FILE "credentials.txt"
#define NOTIFICATION_FILE "notifications.txt"

struct notification {
    int id;
    char title[50];
    char message[200];
    int read; 
};

char currentUser[20];
char currentRole[20];

volatile int running = 0;        
pthread_t notifierThread;
pthread_mutex_t fileMutex = PTHREAD_MUTEX_INITIALIZER;

void ensureFiles();
void signupUser();
int loginSystem();
void mainMenu();
void adminMenu();
void userMenu();

void addNotification();
void updateNotification();
void deleteNotification();
void markNotificationRead();

void displayAllNotifications();
void displayUnreadNotifications();
void searchNotificationByID();
void searchNotificationByTitle();
void printNotificationHeader();
void printSingleNotification(struct notification nt);

void *notificationWatcher(void *arg);
int countUnreadNotifications();

int main() {
    int choice;
    ensureFiles();   
    while (1) {
        printf("\n=========== SMART NOTIFICATION MANAGER ===========\n");
        printf("1. Signup\n");
        printf("2. Login\n");
        printf("3. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                signupUser();
                break;

            case 2:
                if (loginSystem()) {
                    running = 1;
                    if (pthread_create(&notifierThread, NULL,
                                      notificationWatcher, NULL) != 0) {
                        printf("Error creating background thread!\n");
                        running = 0;
                    }
                    mainMenu();
                    running = 0;
                    if (notifierThread) {
                        pthread_join(notifierThread, NULL);
                    }
                } else {
                    printf("Login failed after 3 attempts.\n");
                }
                break;

            case 3:
                printf("Exiting program...\n");
                return 0;

            default:
                printf("Invalid choice!\n");
        }
    }
}

void ensureFiles() {
    FILE *fp;

    fp = fopen(CREDENTIAL_FILE, "a");
    if (!fp) {
        printf("Error creating/opening credential file!\n");
        exit(1);
    }
    fclose(fp);

    fp = fopen(NOTIFICATION_FILE, "a");
    if (!fp) {
        printf("Error creating/opening notification file!\n");
        exit(1);
    }
    fclose(fp);
}

void signupUser() {
    char user[20], pass[20], role[10];
    char fileUser[20], filePass[20], fileRole[10];
    int exists = 0;

    printf("\n=========== SIGNUP ===========\n");
    printf("Enter new username: ");
    scanf("%s", user);

    FILE *fp = fopen(CREDENTIAL_FILE, "r");
    if (!fp) {
        printf("Error opening credential file!\n");
        return;
    }

    while (fscanf(fp, "%s %s %s", fileUser, filePass, fileRole) != EOF) {
        if (strcmp(user, fileUser) == 0) {
            exists = 1;
            break;
        }
    }
    fclose(fp);

    if (exists) {
        printf("Username already exists. Choose another.\n");
        return;
    }

    printf("Enter new password: ");
    scanf("%s", pass);

    printf("Enter role (admin/user): ");
    scanf("%s", role);

    fp = fopen(CREDENTIAL_FILE, "a");
    if (!fp) {
        printf("Error opening credential file!\n");
        return;
    }

    fprintf(fp, "%s %s %s\n", user, pass, role);
    fclose(fp);

    printf("Signup successful! You can login now.\n");
}

int loginSystem() {
    char username[20], password[20];
    char fileUser[20], filePass[20], fileRole[20];
    int attempts = 0;
    int success = 0;

    while (attempts < 3 && !success) {
        printf("\n================ LOGIN ================\n");
        printf("Enter username: ");
        scanf("%s", username);
        printf("Enter password: ");
        scanf("%s", password);

        FILE *fp = fopen(CREDENTIAL_FILE, "r");
        if (!fp) {
            printf("Credential file missing!\n");
            return 0;
        }

        while (fscanf(fp, "%s %s %s", fileUser, filePass, fileRole) != EOF) {
            if (strcmp(username, fileUser) == 0 &&
                strcmp(password, filePass) == 0) {

                strcpy(currentUser, fileUser);
                strcpy(currentRole, fileRole);
                success = 1;
                break;
            }
        }
        fclose(fp);

        if (!success) {
            attempts++;
            printf("Invalid credentials! Attempts left: %d\n",
                   3 - attempts);
        }
    }

    if (success) {
        printf("Login successful! User: %s  Role: %s\n",
               currentUser, currentRole);
    }

    return success;
}

void mainMenu() {
    if (strcmp(currentRole, "admin") == 0) {
        adminMenu();
    } else {
        userMenu();
    }
}

void adminMenu() {
    int ch;

    while (1) {
        printf("\n=============== ADMIN MENU ===============\n");
        printf("1. Add Notification\n");
        printf("2. Display All Notifications\n");
        printf("3. Display Unread Notifications\n");
        printf("4. Search by ID\n");
        printf("5. Search by Title\n");
        printf("6. Update Notification\n");
        printf("7. Delete Notification\n");
        printf("8. Logout\n");
        printf("Enter choice: ");
        scanf("%d", &ch);

        switch (ch) {
            case 1: addNotification(); break;
            case 2: displayAllNotifications(); break;
            case 3: displayUnreadNotifications(); break;
            case 4: searchNotificationByID(); break;
            case 5: searchNotificationByTitle(); break;
            case 6: updateNotification(); break;
            case 7: deleteNotification(); break;
            case 8: return;
            default: printf("Invalid choice!\n");
        }
    }
}

void userMenu() {
    int ch;

    while (1) {
        printf("\n================ USER MENU ===============\n");
        printf("1. Display All Notifications\n");
        printf("2. Display Unread Notifications\n");
        printf("3. Search by ID\n");
        printf("4. Search by Title\n");
        printf("5. Mark Notification as Read\n");
        printf("6. Logout\n");
        printf("Enter choice: ");
        scanf("%d", &ch);

        switch (ch) {
            case 1: displayAllNotifications(); break;
            case 2: displayUnreadNotifications(); break;
            case 3: searchNotificationByID(); break;
            case 4: searchNotificationByTitle(); break;
            case 5: markNotificationRead(); break;
            case 6: return;
            default: printf("Invalid choice!\n");
        }
    }
}
void printNotificationHeader() {
    printf("\nID\tTitle\tMessage\tStatus\n");
    printf("-------------------------------------------\n");
}

void printSingleNotification(struct notification nt) {
    printf("%d\t%s\t%s\t%s\n",
           nt.id,
           nt.title,
           nt.message,
           nt.read ? "READ" : "UNREAD");
}
void addNotification() {
    struct notification nt;
    FILE *fp = fopen(NOTIFICATION_FILE, "a");

    if (!fp) {
        printf("Error opening notification file!\n");
        return;
    }

    printf("Enter notification ID: ");
    scanf("%d", &nt.id);

    printf("Enter title (single word): ");
    scanf("%s", nt.title);

    printf("Enter message (single word): ");
    scanf("%s", nt.message);

    nt.read = 0;

    pthread_mutex_lock(&fileMutex);
    fprintf(fp, "%d %s %s %d\n",
            nt.id, nt.title, nt.message, nt.read);
    pthread_mutex_unlock(&fileMutex);

    fclose(fp);

    printf("Notification added.\n");
}
void displayAllNotifications() {
    struct notification nt;
    FILE *fp = fopen(NOTIFICATION_FILE, "r");

    if (!fp) {
        printf("Error opening notification file!\n");
        return;
    }

    pthread_mutex_lock(&fileMutex);
    printNotificationHeader();

    while (fscanf(fp, "%d %s %s %d",
                  &nt.id, nt.title, nt.message, &nt.read) != EOF) {
        printSingleNotification(nt);
    }

    pthread_mutex_unlock(&fileMutex);
    fclose(fp);
}

void displayUnreadNotifications() {
    struct notification nt;
    FILE *fp = fopen(NOTIFICATION_FILE, "r");
    int found = 0;

    if (!fp) {
        printf("Error opening notification file!\n");
        return;
    }

    pthread_mutex_lock(&fileMutex);
    printNotificationHeader();

    while (fscanf(fp, "%d %s %s %d",
                  &nt.id, nt.title, nt.message, &nt.read) != EOF) {
        if (nt.read == 0) {
            printSingleNotification(nt);
            found = 1;
        }
    }

    pthread_mutex_unlock(&fileMutex);
    fclose(fp);

    if (!found) {
        printf("No unread notifications.\n");
    }
}

void searchNotificationByID() {
    int id;
    struct notification nt;
    FILE *fp = fopen(NOTIFICATION_FILE, "r");
    int found = 0;

    if (!fp) {
        printf("Error opening notification file!\n");
        return;
    }

    printf("Enter notification ID to search: ");
    scanf("%d", &id);

    pthread_mutex_lock(&fileMutex);

    while (fscanf(fp, "%d %s %s %d",
                  &nt.id, nt.title, nt.message, &nt.read) != EOF) {
        if (nt.id == id) {
            printNotificationHeader();
            printSingleNotification(nt);
            found = 1;
            break;
        }
    }

    pthread_mutex_unlock(&fileMutex);
    fclose(fp);

    if (!found) {
        printf("Notification with ID %d not found.\n", id);
    }
}

void searchNotificationByTitle() {
    char title[50];
    struct notification nt;
    FILE *fp = fopen(NOTIFICATION_FILE, "r");
    int found = 0;

    if (!fp) {
        printf("Error opening notification file!\n");
        return;
    }

    printf("Enter title to search: ");
    scanf("%s", title);

    pthread_mutex_lock(&fileMutex);
    printNotificationHeader();

    while (fscanf(fp, "%d %s %s %d",
                  &nt.id, nt.title, nt.message, &nt.read) != EOF) {
        if (strcmp(nt.title, title) == 0) {
            printSingleNotification(nt);
            found = 1;
        }
    }

    pthread_mutex_unlock(&fileMutex);
    fclose(fp);

    if (!found) {
        printf("No notification found with title %s.\n", title);
    }
}

void updateNotification() {
    int id;
    struct notification nt;
    int found = 0;

    printf("Enter notification ID to update: ");
    scanf("%d", &id);

    FILE *fp = fopen(NOTIFICATION_FILE, "r");
    FILE *tmp = fopen("temp.txt", "w");

    if (!fp || !tmp) {
        printf("Error opening files for update!\n");
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        return;
    }

    pthread_mutex_lock(&fileMutex);

    while (fscanf(fp, "%d %s %s %d",
                  &nt.id, nt.title, nt.message, &nt.read) != EOF) {
        if (nt.id == id) {
            found = 1;
            printf("Enter new title (single word): ");
            scanf("%s", nt.title);
            printf("Enter new message (single word): ");
            scanf("%s", nt.message);
        }
        fprintf(tmp, "%d %s %s %d\n",
                nt.id, nt.title, nt.message, nt.read);
    }

    pthread_mutex_unlock(&fileMutex);

    fclose(fp);
    fclose(tmp);

    if (found) {
        remove(NOTIFICATION_FILE);
        rename("temp.txt", NOTIFICATION_FILE);
        printf("Notification updated.\n");
    } else {
        remove("temp.txt");
        printf("Notification ID not found.\n");
    }
}

void deleteNotification() {
    int id;
    struct notification nt;
    int found = 0;

    printf("Enter notification ID to delete: ");
    scanf("%d", &id);

    FILE *fp = fopen(NOTIFICATION_FILE, "r");
    FILE *tmp = fopen("temp.txt", "w");

    if (!fp || !tmp) {
        printf("Error opening files for delete!\n");
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        return;
    }

    pthread_mutex_lock(&fileMutex);

    while (fscanf(fp, "%d %s %s %d",
                  &nt.id, nt.title, nt.message, &nt.read) != EOF) {
        if (nt.id == id) {
            found = 1;
            continue;  
        }
        fprintf(tmp, "%d %s %s %d\n",
                nt.id, nt.title, nt.message, nt.read);
    }

    pthread_mutex_unlock(&fileMutex);

    fclose(fp);
    fclose(tmp);

    if (found) {
        remove(NOTIFICATION_FILE);
        rename("temp.txt", NOTIFICATION_FILE);
        printf("Notification deleted.\n");
    } else {
        remove("temp.txt");
        printf("Notification ID not found.\n");
    }
}

void markNotificationRead() {
    int id;
    struct notification nt;
    int found = 0;

    printf("Enter notification ID to mark as read: ");
    scanf("%d", &id);

    FILE *fp = fopen(NOTIFICATION_FILE, "r");
    FILE *tmp = fopen("temp.txt", "w");

    if (!fp || !tmp) {
        printf("Error opening files for mark-read!\n");
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        return;
    }

    pthread_mutex_lock(&fileMutex);

    while (fscanf(fp, "%d %s %s %d",
                  &nt.id, nt.title, nt.message, &nt.read) != EOF) {
        if (nt.id == id) {
            nt.read = 1;
            found = 1;
        }
        fprintf(tmp, "%d %s %s %d\n",
                nt.id, nt.title, nt.message, nt.read);
    }

    pthread_mutex_unlock(&fileMutex);

    fclose(fp);
    fclose(tmp);

    if (found) {
        remove(NOTIFICATION_FILE);
        rename("temp.txt", NOTIFICATION_FILE);
        printf("Notification marked as read.\n");
    } else {
        remove("temp.txt");
        printf("Notification ID not found.\n");
    }
}

int countUnreadNotifications() {
    struct notification nt;
    FILE *fp = fopen(NOTIFICATION_FILE, "r");
    int count = 0;

    if (!fp) {
        return 0;
    }

    while (fscanf(fp, "%d %s %s %d",
                  &nt.id, nt.title, nt.message, &nt.read) != EOF) {
        if (nt.read == 0) {
            count++;
        }
    }

    fclose(fp);
    return count;
}

void *notificationWatcher(void *arg) {
    while (running) {
        pthread_mutex_lock(&fileMutex);
        int unread = countUnreadNotifications();
        pthread_mutex_unlock(&fileMutex);

        
    }
    return NULL;
}
