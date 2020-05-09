//
// Created by sids on 1/24/19.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

//High Level Functions
int encoder(char *argv[], int inlineFlagIndicator, int inlineMessageLength);

int decoder(char *argv[], int inlineFlagIndicator, int inlinePPLen);

//Facilitating Functions
long long int passPhraseCalculate(char *argv[], int inlineFlagIndicator, int inlinePassPhraseCharCount);

long long int sunCodePassPhrase(long long int passPhrase);

int actCode(int digit);

int writeOutput(long long int outputMessage, int outputLen, char *argv[]);

//Short, Custom Support functions
int calculateIntLen(long long int input);

unsigned concatenate(unsigned x, unsigned y);

int main(int argc, char *argv[]) {
    int inlineFlag = 0;//Assume the user wants to use file mode.
    if (argc >= 3) { //TODO: Replace 3 with 5 before release, this is for development ONLY.
        if (!strcmp(argv[1], "-e") || !strcmp(argv[1], "--encode") || !strcmp(argv[1], "-encode")) {
            if (!strcmp(argv[2], "-i") || !strcmp(argv[2], "--inline") || !strcmp(argv[2], "-inline")) {
                //Call encoder in in-line mode
                printf("Argv 3 length: %lu\n", strlen(argv[3]));
//                printf("Calling encoder in in-line mode.\n");
                encoder(argv, 1, strlen(argv[3])); //1 is inline mode flag.
            } else {
                //Call encoder in file mode
                printf("Calling encoder in file mode.\n");
                encoder(argv, 0,
                        0);//First zero is file mode flag, second one is inline message len which is 0 as.. well; file mode
            }
            wait(NULL);//Make absolutely sure there are no child processes before termination of program.
            exit(0);
        } else if (!strcmp(argv[1], "-d") || !strcmp(argv[1], "--decode") || !strcmp(argv[1], "-decode")) {
            if (!strcmp(argv[2], "-i") || !strcmp(argv[2], "--inline") || !strcmp(argv[2], "-inline")) {
                //Call decoder in in-line mode
                printf("Calling decoder in in-line mode.\n");
            } else {
                //Call decoder in file mode
                printf("Calling decoder in file mode.\n");
            }
        } else {
            //Call helper, valid argument counts but arguments are invalid
        }
    } else if (argc == 2) {
        if (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version") || !strcmp(argv[1], "-version")) {
            //Print current version
            printf("Will print version.\n");
        } else if (!strcmp(argv[1], "-u") || !strcmp(argv[1], "--upgrade") || !strcmp(argv[1], "-upgrade")) {
            //Call upgrade function
            printf("Will call upgrader.\n");
        } else {
            //Call helper
            printf("Calling helper as arguments didn't match.\n");
        }
    } else {
        //Call helper
        printf("Calling helper.\n");
    }
    return 0;
}

int encoder(char *argv[], int inlineFlagIndicator, int inlineMessageLength) {
    FILE *messageFile;
    int a = 7260703, passPhrasePipe[2], sunPassPhrasePipe[2];
    char messageChar, message[(inlineMessageLength +
                               1)];//TODO: Remove inline Message len and message array; we're not using it
    long long int messageIndividualEncoded, messageASCII, passPhrase, sunPassPhrase, forkFlag = 1;
    //Looping variables
    int i, n;
    printf("Hi, said the encoder.\n");
    //PID variables
    int childPID;
    if (pipe(passPhrasePipe) == -1) {
        fprintf(stderr, "PassPhrase Pipe Failed.\n");
        exit(1);
    } else {
        printf("Pipe created.\n");
    }
    if (pipe(sunPassPhrasePipe) == -1) {
        fprintf(stderr, "sunPassPhrase Pipe Failed\n");
        exit(1);
    } else {
        printf("Pipe 2 created.\n");
    }
    childPID = fork();
    if (childPID == 0) {//Child
        printf("Child: %d\n", childPID);
        close(passPhrasePipe[0]);
        close(sunPassPhrasePipe[0]);
        if (inlineFlagIndicator == 1) {
            i = strlen(argv[4]);
        } else {
            i = -1;
        }
        passPhrase = passPhraseCalculate(argv, inlineFlagIndicator, i);
        write(passPhrasePipe[1], &passPhrase, sizeof(passPhrase));
        close(passPhrasePipe[1]);
        sunPassPhrase = sunCodePassPhrase(passPhrase);
        write(sunPassPhrasePipe[1], &sunPassPhrase, sizeof(sunPassPhrase));
        close(sunPassPhrasePipe[1]);
        exit(0);
    } else if (childPID == -1) {
        //Fallback to parent for doing these, and print a warning in stderr
        passPhrase = passPhraseCalculate(argv, inlineFlagIndicator, i);
        sunPassPhrase = sunCodePassPhrase(passPhrase);
        forkFlag = 0;//Have a flag turned to flag if falling back to single process mode so we don't override passPhrase from pipe reads.inlineMessageLength
    } else {
        //CLose the pipe's writing end.
        close(passPhrasePipe[1]);
        close(sunPassPhrasePipe[1]);
        printf("Closed the pipes.\n");
    }
    /*close(passPhrasePipe[1]);
    close(sunPassPhrasePipe[1]);*/
    if (inlineFlagIndicator == 1) {
        printf("Inline\n");
        messageFile = fopen(argv[3], "r");
        if (messageFile != NULL) {
            fprintf(stderr, "%s",
                    "There is a file with the same name as your inline message, assuming you don't want file mode and using input as message.\n");
        }
        fclose(messageFile);
        n = 0;
        messageChar = argv[3][n];
        printf("%c\n", argv[3][n]);
    } else if (inlineFlagIndicator == 0) { //No explicit prep for in-line needed.
        messageFile = fopen(argv[2], "r"); //Don't load message file if it's file mode.
        if (messageFile == NULL) {
            printf("Couldn't open message file, please make sure the file path is correct.\n");
        } else {
            messageChar = fgetc(messageFile);
            if (messageChar == EOF) {
                printf("The input message file is empty, exiting.\n");
                wait(NULL);//Make sure we don't make the child a zombie!
                exit(1);
            }
        }
    } else { //This should, ideally; NEVER execute.
        printf("Mayday! Mayday! Something has gone horribly wrong, Expected inline flag to either be 1  or 0; but; got: %d . please report this.\n",
               inlineFlagIndicator);
    }
    if (forkFlag == 1) { //Do this here instead of in loop so we don't check for condition a lot of times.
        wait(NULL);
        read(passPhrasePipe[0], &passPhrase, sizeof(passPhrase));
        close(passPhrasePipe[0]);
        read(sunPassPhrasePipe[0], &sunPassPhrase, sizeof(sunPassPhrase));
        close(passPhrasePipe[0]);
        printf("Read pass: %lld\n", passPhrase);
        printf("Read spp: %lld\n", sunPassPhrase);
    }
    while ((inlineFlagIndicator == 1 && messageChar != '\0') || (inlineFlagIndicator == 0 && messageChar != EOF)) {
        printf("Char: %c\n", messageChar);
        messageASCII = (int) messageChar;
        messageIndividualEncoded = (long long int) ((a / passPhrase + messageASCII * sunPassPhrase));
        wait(NULL);//Make sure previous child process has terminated before creating one again.
        childPID = fork();
        if (childPID == 0) { //Create a child process which is responsible for calling write out
            fclose(messageFile);
            printf("Calling out.. with %lld\n", messageIndividualEncoded);
            i = calculateIntLen(messageIndividualEncoded);
            writeOutput(messageIndividualEncoded, i, argv);
            printf("Out finished.\n");
            exit(0);
        } else if (childPID == -1) {
            //Get the parent to write output, and print a warning in stderr
            fprintf(stderr, "%s",
                    "Could not create child for writing the output, getting the parent to do it.. It may impact the performance a little.\n");
            writeOutput(messageIndividualEncoded, calculateIntLen(messageIndividualEncoded), argv);
        }
//        writeOutput(messageIndividualEncoded, i, argv);
        if (inlineFlagIndicator == 1) {
            n++;
            messageChar = argv[3][n];
        } else {
            messageChar = fgetc(messageFile);
            if (messageChar == EOF) {
                printf("End of file.\n");
            }
        }
    }
    if (inlineFlagIndicator == 0) {
        fclose(messageFile);//We good debs, we close file if we open it
    }
    return 0;
}

int decoder(char *argv[], int inlineFlagIndicator, int inlineMessageLength) {//TODO: Stop taking the message len
    FILE *messageFile;
    int a = 7260703, passPhrasePipe[2], sunPassPhrasePipe[2];
    char messageChar;
    long long int originalASCII, messageASCII, passPhrase, sunPassPhrase, codeToDecode;
    int flag, forkFlag = 1;
    //Looping variables
    int i, n;
    printf("Hi, said the decoder.\n");
    //PID variables
    int childPID;
    if (pipe(passPhrasePipe) == -1) {
        fprintf(stderr, "PassPhrase Pipe Failed.\n");
        exit(1);
    } else {
        printf("Pipe created.\n");
    }
    if (pipe(sunPassPhrasePipe) == -1) {
        fprintf(stderr, "sunPassPhrase Pipe Failed\n");
        exit(1);
    } else {
        printf("Pipe 2 created.\n");
    }
    childPID = fork();
    if (childPID == 0) {//Child
        printf("Child: %d\n", childPID);
        close(passPhrasePipe[0]);
        close(sunPassPhrasePipe[0]);
        if (inlineFlagIndicator == 1) {
            i = strlen(argv[4]);
        } else {
            i = -1;
        }
        passPhrase = passPhraseCalculate(argv, inlineFlagIndicator, i);
        write(passPhrasePipe[1], &passPhrase, sizeof(passPhrase));
        close(passPhrasePipe[1]);
        sunPassPhrase = sunCodePassPhrase(passPhrase);
        write(sunPassPhrasePipe[1], &sunPassPhrase, sizeof(sunPassPhrase));
        close(sunPassPhrasePipe[1]);
        exit(0);
    } else if (childPID == -1) {
        //Fallback to parent for doing these, and print a warning in stderr
        passPhrase = passPhraseCalculate(argv, inlineFlagIndicator, i);
        sunPassPhrase = sunCodePassPhrase(passPhrase);
        forkFlag = 0;//Have a flag turned to flag if falling back to single process mode so we don't override passPhrase from pipe reads.
    } else {
        //CLose the pipe's writing end.
        close(passPhrasePipe[1]);
        close(sunPassPhrasePipe[1]);
        printf("Closed the pipes.\n");
    }
    /*close(passPhrasePipe[1]);
    close(sunPassPhrasePipe[1]);*/
    if (inlineFlagIndicator == 1) {
        printf("Inline\n");
        messageFile = fopen(argv[3], "r");
        if (messageFile != NULL) {
            fprintf(stderr, "%s",
                    "There is a file with the same name as your inline message, assuming you don't want file mode and using input as message.\n");
        }
        fclose(messageFile);
        n = 0;
        messageChar = argv[3][n];
        printf("%c\n", argv[3][n]);
    } else if (inlineFlagIndicator == 0) { //No explicit prep for in-line needed.
        messageFile = fopen(argv[2], "r"); //Don't load message file if it's file mode.
        if (messageFile == NULL) {
            printf("Couldn't open message file, please make sure the file path is correct.\n");
        } else {
            messageChar = fgetc(messageFile);
            if (messageChar == EOF) {
                printf("The input message file is empty, exiting.\n");
                wait(NULL);//Make sure we don't make the child a zombie!
                exit(1);
            }
        }
    } else { //This should, ideally; NEVER execute.
        printf("Mayday! Mayday! Something has gone horribly wrong, Expected inline flag to either be 1  or 0; but; got: %d . please report this.\n",
               inlineFlagIndicator);
    }
    if (forkFlag == 1) { //Do this here instead of in loop so we don't check for condition a lot of times.
        wait(NULL);
        read(passPhrasePipe[0], &passPhrase, sizeof(passPhrase));
        close(passPhrasePipe[0]);
        read(sunPassPhrasePipe[0], &sunPassPhrase, sizeof(sunPassPhrase));
        close(passPhrasePipe[0]);
        printf("Read pass: %lld\n", passPhrase);
        printf("Read spp: %lld\n", sunPassPhrase);
    }
    while ((inlineFlagIndicator == 1 && messageChar != '\0') || (inlineFlagIndicator == 0 && messageChar != EOF)) {
        if (messageChar != '-' || messageChar != EOF) {
            messageChar = messageChar - '0'; //converts to int on which we can operate
            if (flag == 1) {
                codeToDecode = messageChar;
                flag = 0;
            } else {
                codeToDecode = concatenate(codeToDecode, messageChar);
            }
        } else {
            originalASCII = ((codeToDecode - (a / passPhrase)) / sunPassPhrase);
            wait(NULL);//Make sure previous child process has terminated before creating one again.
            childPID = fork();
            if (childPID == 0) { //Create a child process which is responsible for calling write out
                fclose(messageFile);
                printf("Calling out.. with %lld\n", originalASCII);
                i = calculateIntLen(originalASCII);
                writeOutput(originalASCII, i, argv);
                printf("Out finished.\n");
                exit(0);
            } else if (childPID == -1) {
                //Get the parent to write output, and print a warning in stderr
                fprintf(stderr, "%s",
                        "Could not create child for writing the output, getting the parent to do it.. It may impact the performance a little.\n");
                writeOutput(originalASCII, calculateIntLen(originalASCII), argv);
            }
            flag = 1;
        }
        if (inlineFlagIndicator == 1) {
            n++;
            messageChar = argv[3][n];
        } else {
            messageChar = fgetc(messageFile);
            if (messageChar == EOF) {
                printf("End of file.\n");
            }
        }
    }
    if (inlineFlagIndicator == 0) {
        fclose(messageFile);//We good debs, we close file if we open it
    }
    return 0;
}

long long int passPhraseCalculate(char *argv[], int inlineFlagIndicator, int inlinePassPhraseCharCount) {
    FILE *passPhraseFile;
    char passPhraseChar;
    char passPhraseCharacters[inlinePassPhraseCharCount + 1];
    long long int passPhraseASCII, passPhrase, passPhraseDiff, flag = 1, countPPDigit = 0;
    //Looping variables
    int i, n;
    //Start Code for PP calculation
    if (inlineFlagIndicator == 1) {//&& argv[4][0] == '#'
        passPhraseFile = fopen(argv[4], "r");
        if (passPhraseFile != NULL) {
            fprintf(stderr, "%s",
                    "There is a file with the same name as your passphrase, assuming you don't want to use the file and using passphrase input as message.\n");
        }
        fclose(passPhraseFile);//You know what they say,outputFile, if you flip it, flop it. Oh; wait, they don't? well that's too bad, isn't it?
        n = 0;//Replace with 1 if case of begin with "#"
        passPhraseChar = argv[4][n];
    } else {
        passPhraseFile = fopen(argv[3], "r");
        if (passPhraseFile == NULL) {
            printf("Please give valid passphrase file destination.\n");
            exit(1);
        } else {
            passPhraseChar = fgetc(passPhraseFile);
            if (passPhraseChar == EOF) {
                printf("The passphrase file is empty, exiting.\n");
                exit(1);
            }
        }
    }
    while ((inlineFlagIndicator == 1 && passPhraseChar != '\0') ||
           (inlineFlagIndicator == 0 && passPhraseChar != EOF)) {
//        if ((int) passPhraseChar != 10) { //NO idea why i was doing this.. it's a major issue as referenced here: https://github.com/Sid-Sun/codeMKII/issues/2
        passPhraseASCII = (int) passPhraseChar;
        countPPDigit = countPPDigit + calculateIntLen(passPhraseASCII);
        if (flag == 1) {
            passPhraseDiff = passPhraseASCII;
            flag = 0;
        } else {
            passPhraseDiff = passPhraseDiff - passPhraseASCII;
        }
        if (inlineFlagIndicator == 1) {
            n++;
            passPhraseChar = argv[4][n];
        } else {
            passPhraseChar = fgetc(passPhraseFile);
        }
    }
    if (inlineFlagIndicator == 0) {
        fclose(passPhraseFile);
    }
    passPhraseDiff = llabs(passPhraseDiff);//Get ABSOLUTE value of diff.
    passPhrase = (passPhraseDiff / countPPDigit);
    printf("pasPhrase: %lld\n", passPhrase);
    return passPhrase;
}

long long int sunCodePassPhrase(long long int passPhrase) {
    int count = calculateIntLen(passPhrase);
    int digit, code, spp, flag = 1, digitArray[count], n = 1;
    while (passPhrase != 0) {
        digit = passPhrase % 10;
        digitArray[count - n] = digit; //n must be initialized with 1 as arrays start at 0
        n++;
        passPhrase = passPhrase / 10;
    }
    for (n = 0; n < count; n++) {
        code = actCode(digitArray[n]);
        if (flag == 1) {
            spp = code;
            flag = 0;
        } else {
            spp = concatenate(spp, code);
        }
    }
    printf("SPP: %d\n", spp);
    return spp;
}

int actCode(int digit) {
    switch (digit) {
        case 0:
            return 0;
        case 1:
            return 1;
        case 2:
            return 10;
        case 3:
            return 11;
        case 4:
            return 100;
        case 5:
            return 101;
        case 6:
            return 110;
        case 7:
            return 111;
        case 8:
            return 1000;
        case 9:
            return 1001;
        default:
            printf("Expected input b/w 1->10 for actCode but received: %d; please report this, exiting...", digit);
            exit(1);
    }
}

int writeOutput(long long int outputMessage, int outputLen, char *argv[]) {
    printf("WOW");
    int i;
    FILE *outputFile;
    int encodedMessageLength, encodedMessageDigitArray[outputLen], messageDigit, n = 1;//n=1 as arrays start at 0
    if (!strcmp(argv[2], "-i") || !strcmp(argv[2], "--inline") || !strcmp(argv[2], "-inline")) {
        if (!strcmp(argv[5], "stdout")) {
            printf("Stdout, inline");
            outputFile = stdout;
        } else {
            printf("The output must be at 5\n");
            outputFile = fopen(argv[5], "a");
        }
    } else {
        if (!strcmp(argv[4], "stdout")) {
            printf("Stdout, not inline");
            outputFile = stdout;
        } else {
            printf("The out is at 4\n");
            outputFile = fopen(argv[4], "a");
        }
    }
    printf("Oh, encoder.. Recd: %lld\n", outputMessage);
    if (!strcmp(argv[1], "-e") || !strcmp(argv[1], "--encode") || !strcmp(argv[1], "-encode")) {
        while (outputMessage != 0) {//Get the digits in the array.
            messageDigit = outputMessage % 10;
            encodedMessageDigitArray[outputLen - n] = messageDigit;
            n++;
            outputMessage = outputMessage / 10;
        }
        for (i = 0; i < outputLen; i++) {
            fprintf(outputFile, "%d", encodedMessageDigitArray[i]);
        }
        fprintf(outputFile, "%c", '-');
    } else if (!strcmp(argv[1], "-d") || !strcmp(argv[1], "--decode") || !strcmp(argv[1], "-decode")) {
        fprintf(outputFile, "%c", outputMessage);
    }
    if (strcmp(argv[4], "stdout") || strcmp(argv[5], "stdout")) {//close output file if it was opened
        fclose(outputFile);
    }
    return 0;
}

//Support functions
int calculateIntLen(long long int input) {
    int count = 0;
    while (input != 0) {
        input = input / 10;
        count++;
    }
    return count;
}

unsigned concatenate(unsigned x, unsigned y) {
    unsigned pow = 10;
    while (y >= pow)
        pow *= 10;
    return x * pow + y;
}