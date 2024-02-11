#include <SDL.h>
#include <stdlib.h>
#include <IOBluetooth/Bluetooth.h>
#include <stdio.h>
#include <sys/stat.h>
#include <termios.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <unistd.h>

struct Data {
    uint8_t knapp;
    uint8_t value;
};
struct termios serialConfig;
int serialPort; // fd till serieporten
char magicPacket[] = {'z', 'z'};
// Macro för att köra en funktion och säga till om den failar
void assertFail(char* expr, char* file, int line, char* msg) {
    char* errorMsg = (char*)malloc(1024);
    snprintf(errorMsg, 1023, "Fel i %s:%i.\nExpression: %s\nErrno: %s\nMeddelande: %s", file, line, expr, strerror(errno), msg);
    puts(errorMsg);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fel", errorMsg, nullptr);
    free(errorMsg);
    exit(1);
}
#define assert(expr, msg) if(!(expr)) assertFail((char*)#expr, (char*)__FILE__, __LINE__, (char*)msg);
#define assertZeroErrormsg(expr, msg) assert(!(expr), msg);
// https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/
int initTTY() {
    serialPort = open("/dev/tty.HC-05", O_RDWR);
    fprintf(stderr, "FD: %d\n", serialPort);
    assert(serialPort != -1, "Serieporten kan inte öppnas... Se till att screen inte är öppet");
    assertZeroErrormsg(tcgetattr(serialPort, &serialConfig), "Kan inte ansluta få serie-configen från hc-05");
    // ta bort plarity bitten
    serialConfig.c_cflag &= ~PARENB;
    // stäng av att den försöker att tolka saker som inte ska tolkas
    serialConfig.c_lflag &= ~ISIG;
    serialConfig.c_iflag &= ~(IXON | IXOFF | IXANY);
    serialConfig.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
    // sätt rätt baud rate (9600)
    assertZeroErrormsg(cfsetispeed(&serialConfig, B9600), "kan inte sätta input-hastigheten till hc-05");
    assertZeroErrormsg(cfsetospeed(&serialConfig, B9600), "kan inte sätta output-hastigheten till hc-05");
    assertZeroErrormsg(tcsetattr(serialPort, TCSANOW, &serialConfig), "kan inte applya settingsen till hc-05");
    return 0;
}
void sendMagicPacket() {
    assert(write(serialPort, &magicPacket, sizeof(magicPacket)) != -1, "kan inte skicka det magiska paketet");
}
uint8_t latestValues[256];
void sendEvent(struct Data val) {
    latestValues[val.knapp] = val.value;
    puts("Skickar data!");
    assert(write(serialPort, &val, sizeof(struct Data)) != -1, "kan inte skicka kontrollerdata");
}
SDL_Joystick* joystick;
SDL_Window* window = nullptr;
void controllerLoop() {
loopbegin:
    joystick = SDL_JoystickOpen(0);
    if(joystick == nullptr) {
        int knappTryckt;
        SDL_MessageBoxButtonData buttons[2] = {
            {.buttonid = 0, .text = "Försök igen", .flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT},
            {.buttonid = 1, .text = "Jag har ingen kontroll", .flags = SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT}
        };
        SDL_MessageBoxData msgboxdata = {
            .window = nullptr,
            .title = "Ingen kontroller",
            .flags = SDL_MESSAGEBOX_ERROR,
            .message = "Jag tycker att det är ganska rimligt att man borde ha en kontroll för ett program som styr med en kontroll",
            .buttons = buttons,
            .numbuttons = 2
        };
        SDL_ShowMessageBox(&msgboxdata, &knappTryckt);
        if(knappTryckt == 1) exit(1);
        goto loopbegin;
    }
}
int main() {
    // Skapa sdl med video och kontroller-support
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
    SDL_Renderer* renderer = nullptr;
    SDL_CreateWindowAndRenderer(320, 240, SDL_WINDOW_SHOWN, &window, &renderer);
    SDL_SetWindowTitle(window, "Bilkontroll");
    controllerLoop();
    fprintf(stderr, "Kontroller: %s\n", SDL_JoystickNameForIndex(0));
    if(initTTY() != 0) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Ingen serieport", "Kunde inte sätta upp serieporten", window);
        return 1;
    }
    bool run = true;
    sendMagicPacket();
    while(run) {
        SDL_Event e;
        SDL_WaitEvent(&e);
        if (e.type == SDL_QUIT) run = false;
        else if (e.type == SDL_KEYDOWN) {
            //printf("keydown!!!\n");
        }
        else if (e.type == SDL_JOYAXISMOTION) {
            if(abs(e.jaxis.value) < 3000) e.jaxis.value = 0;
            // // skicka inte samma sak igen, hjälper till ifall att kontrollen driftar
            uint8_t binaryValue = (e.jaxis.value/256)+128;
            if(latestValues[e.jaxis.axis] == binaryValue) continue;
            printf("axis: %i %i\n", e.jaxis.axis, e.jaxis.value);
            struct Data data = {
                .knapp = e.jaxis.axis,
                .value = binaryValue
            };
            sendEvent(data);
            sendMagicPacket();
        }
        else if(e.type == SDL_JOYBUTTONDOWN) {
            printf("knapp!! %i\n", e.jbutton.button);
            struct Data data = {
                .knapp = e.jbutton.button + 100,
                .value = 1
            };
            sendEvent(data);
            sendMagicPacket();
        }
        else if(e.type == SDL_JOYDEVICEREMOVED) {
            controllerLoop();
        }
    }
}