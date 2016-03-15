## RUNNING AND USING DMB CYUSB3.0 NT1065.1 DEMO SOFTWARE

### 1. Basic configuration options
Go to the software folder and run Sampler_NT1065.exe.

The program allows you to perform the following operations:
- **Open device** – connect demo-board to demo software. If it is the first launch of demoboard after applying power supply, working firmware will be flashed and the name of demoboard in “Device manager” should change from “Cypress USB BootLoader” to “Cypress USB StreamerExample”. Log message “OpenDevise returned CY3DEV_OK” if connection was successful and “SuperSpeed USB”, if DMB CYUSB3.0 NT1065.1 was connected to USB3.0, or “HighSpeed USB”, if it was connected to USB2.0. In addition, shows connected endpoint parameters.
- **Close device** – disconnect demo-board from demo software.
- **Read ID** – log message of IC ID and revision if connection was
successful and demo-board is ready.

Following options should only be used after writing a correct configuration file to IC NT1065.1:
- **Start stream** – starts data stream transmission from NT1065.1 chip to the computer.
Data sampling speed is displayed below “Read ID” button.
- **Stop stream** – stops data stream transmission from NT1065.1 chip to the computer.


### 2. Data processing options
Following demo options are available in software:
- Calculating ADC filling

  Tick the appropriate check box “Calculate ADC filling” to start measuring. The results are displayed in a log in real time (one measure across 8M samples data chunk).

- Saving data dump to file

  Select the file size and choose a location on your computer where file should be saved. Each sample takes one byte (for 2 bits per 4 channels).

- ADC signal spectrum display

  Tick the appropriate box “Calculate spectrum” to start measure then push the button “Show spectrum” to open new window with spectrum waveform.
  Enable or disable specific channels display with the appropriate check boxes. Set desired frameskip (one frame is 2M samples data chunk, FFT size is 65536 samples per frame) and averager values to improve performance at the expense of visual smoothness. Spectrum display is set for 53MHz ADC sampling rate by default. If another output rate is used, display rate should be changed with "Disp. sample rate" option in main window.
