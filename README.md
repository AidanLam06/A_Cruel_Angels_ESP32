# aCruelAngelsESP32
plays the first few seconds of A Cruel Angel's Thesis over a passive buzzer, activated by a button all configured for an ESP32
--> Build using ESP-IDF by Espressif for my ESP32-WROOM-32 CP2102

https://youtube.com/shorts/NYoSp8OFOuM?si=JgF8j0CW_bYeDHs_ 


![IMG_2836](https://github.com/user-attachments/assets/2a61b6c5-2909-4aaa-97e1-e6169b9754ac)


# Debugging
- First was the button. My initial idea was to make the passive buzzer activate while the button was held but the button's poor quality prevented this from happening, bypassing the timer-callback debouncing, and causing multiple resets of the melody. I didn't want to sacrifice activation speed to make sure I was holding the button down firmly enough, so I opted to switch to a single-press-activation system rather than a press-and-hold system. This allowed the system to run smoother since it only required the button to pass the debouncing once rather than keep continuous pressure on the button, which gets kind of uncomfortable. 
- Next was the passive buzzer. Initially no PWM signal was being sent to the passive buzzer so I used an LED in parallel (because I didn't have an oscilloscope) with the buzzer to check if any voltage was being sent through it, but none appeared to be, so I swapped breadboards and the issue was resolved. It turned out that the nickel plating on the breadboard had lightly oxidized (which was no surprise since I got it from aliexpress for a dollar) and caused an increase in resistance which affected the PWM signal output from the ESP32. 
