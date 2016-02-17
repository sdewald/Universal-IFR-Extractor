#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include "EFI.h"
#include "UEFI.h"

// Global constants

// Handel actions
enum {NO_ACTION, EXTRACT_BUTTON_ACTION, BROWSE_BUTTON_ACTION, FILE_LOCATION_EDIT_ACTION};

// Error codes
enum {IFR_EXTRACTED, FILE_NOT_FOUND, UNKNOWN_PROTOCOL};

// Global definitions
enum type {EFI, UEFI, UNKNOWN};

// Function prototypes

/*
  Name: fileExists
  Purpose: Checks if a file exists
*/
bool fileExists(const string &file);

/*
  Name: readFile
  Purpose: Reads specified file
*/
void readFile(const string &file, const string &buffer);

/*
  Name: getType
  Purpose: Determines if file uses EFI or UEFI IFR protocol
*/
type getType(const string &buffer);

/*
  Name: showResult:
  Purpose: Print Result of extraction
*/
void showResult(int errorcode);

void extract(type protocol, const string &outputFile, string &buffer) {
  vector<string> strings;

  // Clear strings
  strings.clear();

  if (protocol == EFI) {
    vector<EFI_IFR_STRING_PACK> stringPackages;
    vector<EFI_IFR_FORM_SET_PACK> formSets;

    getEFIStringPackages(stringPackages, buffer);
    getEFIStrings(stringPackages, strings, buffer);
    getEFIFormSets(formSets, buffer, stringPackages, strings);

    generateEFIIFRDump(outputFile, stringPackages, formSets, buffer, strings);
  } else if (protocol == UEFI) {
    vector<UEFI_IFR_STRING_PACK> stringPackages;
    vector<UEFI_IFR_FORM_SET_PACK> formSets;

    getUEFIStringPackages(stringPackages, buffer);
    getUEFIStrings(stringPackages, strings, buffer);
    getUEFIFormSets(formSets, buffer, stringPackages, strings);

    generateUEFIIFRDump(outputFile, stringPackages, formSets, buffer, strings);
  }

  showResult(IFR_EXTRACTED);
}

void showResult(int errorCode) {
  // Check if error code is IFR extracted
  if (errorCode == IFR_EXTRACTED) {
    std::cout << "IFR extracted successfully" << std::endl;
  } else {
    string message;

    if (errorCode == FILE_NOT_FOUND)
      message = "File not found";
    else if (errorCode == UNKNOWN_PROTOCOL)
      message = "Unknown protocol detected";

    std::cerr << "ERROR: " << message << std::endl;
  }
}

bool fileExists(const string &file) {
  // Open file
  ifstream fin(file);

  // Return if first characetr doesn't equal EOF
  return fin.peek() != EOF;
}

void readFile(const string &file, string &buffer) {
  // Initialize variables
  ifstream fin(file, ios::binary);

  // Clear buffer
  buffer.clear();

  // Read in file
  while (fin.peek() != EOF)
    buffer.push_back(fin.get());

  // Close file
  fin.close();
}

type getType(const string &buffer) {
  // Go through buffer
  for (unsigned int i = 0; i < buffer.size() - 9; i++) {
    if ((buffer[i] != '\x00' || buffer[i + 1] != '\x00' || buffer[i + 2] != '\x00' || buffer[i + 3] != '\x00')
        && buffer[i + 4] == '\x02'
        && buffer[i + 5] == '\x00'
        && (buffer[i + 6] != '\x00' || buffer[i + 7] != '\x00' || buffer[i + 8] != '\x00' || buffer[i + 9] != '\x00')
        && i + static_cast<unsigned char>(buffer[i + 6])
        + (static_cast<unsigned char>(buffer[i + 7]) << 8)
        + (static_cast<unsigned char>(buffer[i + 8]) << 16)
        + (static_cast<unsigned char>(buffer[i + 9]) << 24) < buffer.size()
        && buffer[i + static_cast<unsigned char>(buffer[i + 6])
                  + (static_cast<unsigned char>(buffer[i + 7]) << 8)
                  + (static_cast<unsigned char>(buffer[i + 8]) << 16)
                  + (static_cast<unsigned char>(buffer[i + 9]) << 24) + 4] >= 'a'
        && buffer[i + static_cast<unsigned char>(buffer[i + 6])
                  + (static_cast<unsigned char>(buffer[i + 7]) << 8)
                  + (static_cast<unsigned char>(buffer[i + 8]) << 16)
                  + (static_cast<unsigned char>(buffer[i + 9]) << 24) + 4] <= 'z'
        && i + static_cast<unsigned char>(buffer[i])
        + (static_cast<unsigned char>(buffer[i + 1]) << 8)
        + (static_cast<unsigned char>(buffer[i + 2]) << 16)
        + (static_cast<unsigned char>(buffer[i + 3]) << 24) < buffer.size()
        && buffer[i + static_cast<unsigned char>(buffer[i])
                  + (static_cast<unsigned char>(buffer[i + 1]) << 8)
                  + (static_cast<unsigned char>(buffer[i + 2]) << 16)
                  + (static_cast<unsigned char>(buffer[i + 3]) << 24) + 4] == '\x02'
        && buffer[i + static_cast<unsigned char>(buffer[i])
                  + (static_cast<unsigned char>(buffer[i + 1]) << 8)
                  + (static_cast<unsigned char>(buffer[i + 2]) << 16)
                  + (static_cast<unsigned char>(buffer[i + 3]) << 24) + 5] == '\x00') {
      return EFI;
    } else if ((buffer[i] != '\x00' || buffer[i + 1] != '\x00' || buffer[i + 2] != '\x00')
               && buffer[i + 3] == '\x04'
               && buffer[i + 4] == '\x34'
               && buffer[i + 44] == '\x01'
               && buffer[i + 45] == '\x00'
               && buffer[i + 48] == '\x2D'
               && i + static_cast<unsigned char>(buffer[i])
               + (static_cast<unsigned char>(buffer[i + 1]) << 8)
               + (static_cast<unsigned char>(buffer[i + 2]) << 16) < buffer.size()
               && buffer[i + static_cast<unsigned char>(buffer[i])
                         + (static_cast<unsigned char>(buffer[i + 1]) << 8)
                         + (static_cast<unsigned char>(buffer[i + 2]) << 16) - 1] == '\x00'
               && buffer[i + static_cast<unsigned char>(buffer[i])
                         + (static_cast<unsigned char>(buffer[i + 1]) << 8)
                         + (static_cast<unsigned char>(buffer[i + 2]) << 16) - 2] == '\x00') {
      return UEFI;
    }
  }

  return UNKNOWN;
}

int main(int argc, char *argv[]) {
  type protocol;

  /* Are src and dest file name arguments missing */
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " file1 file2" << endl;
    return (EXIT_FAILURE);
  }

  // Initialize variables
  string fileLocation = argv[1],
    outputFile = argv[2],
    buffer;

  if (fileExists(fileLocation)) {
    readFile(fileLocation, buffer);
    protocol = getType(buffer);

    if (protocol == UNKNOWN) {
      showResult(UNKNOWN_PROTOCOL);

      return (EXIT_FAILURE);
    }
    
    extract(protocol, outputFile, buffer);
  }

  return (EXIT_SUCCESS);
}
