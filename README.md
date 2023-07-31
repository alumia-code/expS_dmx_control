> ### Developed with and for PhD candidate [Fil Botelho@Orhpeus Institute](https://orpheusinstituut.be/en/orpheus-research-centre/researchers/filipa-botelho)
> This is part of the experimental_system (**expS**) repositories that are present on this github account.

# 🚥dmx_control
A Max-MSP object and patch for DMX serial output.

# 🖱️ Use

Through this object and patch you can control multiple DMX lights simultaneously. Each light can have a different number of DMX channels.

- **You will need (minimum specs):**
>1. Some kind of serial to DMX conversion (we used an arduino with a DMX shield)
>2. As many DMX lights as you want connected in series

- Inside `obj_source` you will find the `C` source code for the Max-MSP object. You will need [Max-msp SDK](https://github.com/Cycling74/max-sdk) if you want to compile from source.
- Inside `examples` you can find one Max patch that showcases the object (`alumia_dmx_control`)
- This object expects some kind of encoded input (in our case, from a bluetooth controller) that will be transformed into DMX messages.
- The patch sends DMX messages to any serial output accessible to Max/MSP. We were using an arduino with a DMX shield to convert these serial messages to the DMX protocol.
- You can find pre-compiled versions as releases

Simply add the object (`alumia_dmx_control`) to the `Packages` folder of Max.

# ☮️ Keep in mind

- Code comments and some other text are in Portuguese (I didn't know about best practices back then)
- This is my first C project (and probably my first programming project ever). The code is not pretty, but it works.
- Please contact me at alumiamusic@gmail.com if you want to use this, or you are interested in the idea, but cannot understand the code/examples/etc.
I am more than happy to help
