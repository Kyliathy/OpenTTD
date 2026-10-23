1. d:\Codez\Sandbox\OpenTTD\docs\zz-reach2\upgrades\2026-10\demo\01OrbitalClutter.png

The orbital construction window has a layout that makes the buttons on the bottom impossible to reach. It has grown more than the full HD viewport on the current machine.

2. Keyboard controls are not working (alt + 4 doesn't open the space layer and repeatedly pressing F2 just continues to show the game settings).


3. 02Stuck.png

The construction probe is stuck, makes no progress

4. Asteroids change direction while on the map. I just saw one that reversed course, ROFLMAO :D. Seriously? Asteroids should continue traveling until they reach the edge of the map. And then, judging by the smarts that coded the reversed direction, they should magically reappear on the other side? :D. No. They should DIE. The object is dumped from the map entirely. Naturally, any probes attached to it would also be lost. Right? NO. Probes would detach before that and head back to the  nearest dump point OR nearest asteroid DEPENDING ON THEIR ORDERS. Probes that are configured, like trains, to wait until they are fully loaded should instead pick the nearest asteroid with the same type of mineral and mine that. So now you're going to hit back and say: yes but asteroids "falling off" the map need to be replaced. And you'd be friggin' right you would :D. And replaced they will be. Instantly. Right? No. Every asteroid that disappears should increment a SAVEABLE counter (as in we need to savegame it) which should say not only how many asteroids have disappeared but also their size. We will respawn asteroids on a random game timer that should randomize between 20 seconds and 60 seconds of real life time. Asteroids that respawn SHOULD NEVER have the same size as the disappearing asteroid. The counter should be increased with +5 for example for larger asteroids. Every respawn interval we respawn an asteroid of a different size than the most recently disappeared one and decrease from the counter its size. Then, we wait until enough counter is accumulated to spawn another one. Obviously this means we also have to save  the type of the last disappearing asteroid. 