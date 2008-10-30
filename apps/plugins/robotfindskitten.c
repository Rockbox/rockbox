/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * robotfindskitten: A Zen simulation
 *
 * Copyright (C) 1997,2000 Leonard Richardson 
 *                         leonardr@segfault.org
 *                         http://www.crummy.com/devel/
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or EXISTANCE OF KITTEN.  See the GNU General
 *   Public License for more details.
 *
 *   http://www.gnu.org/copyleft/gpl.html
 *
 * Ported to Rockbox 2007 by Jonas Häggqvist
 */

#include "plugin.h"
#include "pluginlib_actions.h"

/* This macros must always be included. Should be placed at the top by
   convention, although the actual position doesn't matter */
PLUGIN_HEADER

/*Be sure to change MESSAGES when you change the array, or bad things
  will happen.*/

/*Also, take note that robotfindskitten.c and configure.in 
  currently have the version number hardcoded into them, and they 
  should reflect MESSAGES. */

/* Watch out for fenceposts.*/
#define MESSAGES (sizeof messages / sizeof (char*))

static char* messages[] =
{
  "\"I pity the fool who mistakes me for kitten!\", sez Mr. T.",
  "That's just an old tin can.",
  "It's an altar to the horse god.",
  "A box of dancing mechanical pencils. They dance! They sing!",
  "It's an old Duke Ellington record.",
  "A box of fumigation pellets.",
  "A digital clock. It's stuck at 2:17 PM.",
  "That's just a charred human corpse.",
  "I don't know what that is, but it's not kitten.",
  "An empty shopping bag. Paper or plastic?",
  "Could it be... a big ugly bowling trophy?",
  "A coat hanger hovers in thin air. Odd.",
  "Not kitten, just a packet of Kool-Aid(tm).",
  "A freshly-baked pumpkin pie.",
  "A lone, forgotten comma, sits here, sobbing.",
  "ONE HUNDRED THOUSAND CARPET FIBERS!!!!!",
  "It's Richard Nixon's nose!",
  "It's Lucy Ricardo. \"Aaaah, Ricky!\", she says.",
  "You stumble upon Bill Gates' stand-up act.",
  "Just an autographed copy of the Kama Sutra.",
  "It's the Will Rogers Highway. Who was Will Rogers, anyway?",
  "It's another robot, more advanced in design than you but strangely immobile.",
  "Leonard Richardson is here, asking people to lick him.",
  "It's a stupid mask, fashioned after a beagle.",
  "Your State Farm Insurance(tm) representative!",
  "It's the local draft board.",
  "Seven 1/4\" screws and a piece of plastic.",
  "An 80286 machine.",
  "One of those stupid \"Homes of the Stars\" maps.",
  "A signpost saying \"TO KITTEN\". It points in no particular direction.",
  "A hammock stretched between a tree and a volleyball pole.",
  "A Texas Instruments of Destruction calculator.",
  "It's a dark, amphorous blob of matter.",
  "Just a pincushion.",
  "It's a mighty zombie talking about some love and prosperity.",
  "\"Dear robot, you may have already won our 10 MILLION DOLLAR prize...\"",
  "It's just an object.",
  "A mere collection of pixels.",
  "A badly dented high-hat cymbal lies on its side here.",
  "A marijuana brownie.",
  "A plush Chewbacca.",
  "Daily hunger conditioner from Australasia",
  "Just some stuff.",
  "Why are you touching this when you should be finding kitten?",
  "A glorious fan of peacock feathers.",
  "It's some compromising photos of Babar the Elephant.",
  "A copy of the Weekly World News. Watch out for the chambered nautilus!",
  "It's the proverbial wet blanket.",
  "A \"Get Out of Jail Free\" card.",
  "An incredibly expensive \"Mad About You\" collector plate.",
  "Paul Moyer's necktie.",
  "A haircut and a real job. Now you know where to get one!",
  "An automated robot-hater. It frowns disapprovingly at you.",
  "An automated robot-liker. It smiles at you.",
  "It's a black hole. Don't fall in!",
  "Just a big brick wall.",
  "You found kitten! No, just kidding.",
  "Heart of Darkness brand pistachio nuts.",
  "A smoking branding iron shaped like a 24-pin connector.",
  "It's a Java applet.",
  "An abandoned used-car lot.",
  "A shameless plug for Crummy: http://www.crummy.com/",
  "A shameless plug for the UCLA Linux Users Group: http://linux.ucla.edu/",
  "A can of Spam Lite.",
  "This is another fine mess you've gotten us into, Stanley.",
  "It's scenery for \"Waiting for Godot\".",
  "This grain elevator towers high above you.",
  "A Mentos wrapper.",
  "It's the constellation Pisces.",
  "It's a fly on the wall. Hi, fly!",
  "This kind of looks like kitten, but it's not.",
  "It's a banana! Oh, joy!",
  "A helicopter has crashed here.",
  "Carlos Tarango stands here, doing his best impression of Pat Smear.",
  "A patch of mushrooms grows here.",
  "A patch of grape jelly grows here.",
  "A spindle, and a grindle, and a bucka-wacka-woom!",
  "A geyser sprays water high into the air.",
  "A toenail? What good is a toenail?",
  "You've found the fish! Not that it does you much good in this game.",
  "A Buttertonsils bar.",
  "One of the few remaining discoes.",
  "Ah, the uniform of a Revolutionary-era minuteman.",
  "A punch bowl, filled with punch and lemon slices.",
  "It's nothing but a G-thang, baby.",
  "IT'S ALIVE! AH HA HA HA HA!",
  "This was no boating accident!",
  "Wait! This isn't the poker chip! You've been tricked! DAMN YOU, MENDEZ!",
  "A livery stable! Get your livery!",
  "It's a perpetual immobility machine.",
  "\"On this spot in 1962, Henry Winkler was sick.\"",
  "There's nothing here; it's just an optical illusion.",
  "The World's Biggest Motzah Ball!",
  "A tribe of cannibals lives here. They eat Malt-O-Meal for breakfast, you know.",
  "This appears to be a rather large stack of trashy romance novels.",
  "Look out! Exclamation points!",
  "A herd of wild coffee mugs slumbers here.",
  "It's a limbo bar! How low can you go?",
  "It's the horizon. Now THAT'S weird.",
  "A vase full of artificial flowers is stuck to the floor here.",
  "A large snake bars your way.",
  "A pair of saloon-style doors swing slowly back and forth here.",
  "It's an ordinary bust of Beethoven... but why is it painted green?",
  "It's TV's lovable wisecracking Crow! \"Bite me!\", he says.",
  "Hey, look, it's war. What is it good for? Absolutely nothing. Say it again.",
  "It's the amazing self-referential thing that's not kitten.",
  "A flamboyant feather boa. Now you can dress up like Carol Channing!",
  "\"Sure hope we get some rain soon,\" says Farmer Joe.",
  "\"How in heck can I wash my neck if it ain't gonna rain no more?\" asks Farmer Al.",
  "\"Topsoil's all gone, ma,\" weeps Lil' Greg.",
  "This is a large brown bear. Oddly enough, it's currently peeing in the woods.",
  "A team of arctic explorers is camped here.",
  "This object here appears to be Louis Farrakhan's bow tie.",
  "This is the world-famous Chain of Jockstraps.",
  "A trash compactor, compacting away.",
  "This toaster strudel is riddled with bullet holes!",
  "It's a hologram of a crashed helicopter.",
  "This is a television. On screen you see a robot strangely similar to yourself.",
  "This balogna has a first name, it's R-A-N-C-I-D.",
  "A salmon hatchery? Look again. It's merely a single salmon.",
  "It's a rim shot. Ba-da-boom!",
  "It's creepy and it's kooky, mysterious and spooky. It's also somewhat ooky.",
  "This is an anagram.",
  "This object is like an analogy.",
  "It's a symbol. You see in it a model for all symbols everywhere.",
  "The object pushes back at you.",
  "A traffic signal. It appears to have been recently vandalized.",
  "\"There is no kitten!\" cackles the old crone. You are shocked by her blasphemy.",
  "This is a Lagrange point. Don't come too close now.",
  "The dirty old tramp bemoans the loss of his harmonica.",
  "Look, it's Fanny the Irishman!",
  "What in blazes is this?",
  "It's the instruction manual for a previous version of this game.",
  "A brain cell. Oddly enough, it seems to be functioning.",
  "Tea and/or crumpets.",
  "This jukebox has nothing but Cliff Richards albums in it.",
  "It's a Quaker Oatmeal tube, converted into a drum.",
  "This is a remote control. Being a robot, you keep a wide berth.",
  "It's a roll of industrial-strength copper wire.",
  "Oh boy! Grub! Er, grubs.",
  "A puddle of mud, where the mudskippers play.",
  "Plenty of nothing.",
  "Look at that, it's the Crudmobile.",
  "Just Walter Mattheau and Jack Lemmon.",
  "Two crepes, two crepes in a box.",
  "An autographed copy of \"Primary Colors\", by Anonymous.",
  "Another rabbit? That's three today!",
  "It's a segmentation fault. Core dumped, by the way.",
  "A historical marker showing the actual location of /dev/null.",
  "Thar's Mobius Dick, the convoluted whale. Arrr!",
  "It's a charcoal briquette, smoking away.",
  "A pizza, melting in the sun.",
  "It's a \"HOME ALONE 2: Lost in New York\" novelty cup.",
  "A stack of 7 inch floppies wobbles precariously.",
  "It's nothing but a corrupted floppy. Coaster anyone?",
  "A section of glowing phosphor cells sings a song of radiation to you.",
  "This TRS-80 III is eerily silent.",
  "A toilet bowl occupies this space.",
  "This peg-leg is stuck in a knothole!",
  "It's a solitary vacuum tube.",
  "This corroded robot is clutching a mitten.",
  "\"Hi, I'm Anson Williams, TV's 'Potsy'.\"",
  "This subwoofer was blown out in 1974.",
  "Three half-pennies and a wooden nickel.",
  "It's the missing chapter to \"A Clockwork Orange\".",
  "It's a burrito stand flyer. \"Taqueria El Ranchito\".",
  "This smiling family is happy because they eat LARD.",
  "Roger Avery, persona un famoso de los Estados Unidos.",
  "Ne'er but a potted plant.",
  "A parrot, kipping on its back.",
  "A forgotten telephone switchboard.",
  "A forgotten telephone switchboard operator.",
  "It's an automated robot-disdainer. It pretends you're not there.",
  "It's a portable hole. A sign reads: \"Closed for the winter\".",
  "Just a moldy loaf of bread.",
  "A little glass tub of Carmex. ($.89) Too bad you have no lips.",
  "A Swiss-Army knife. All of its appendages are out. (toothpick lost)",
  "It's a zen simulation, trapped within an ASCII character.",
  "It's a copy of \"The Rubaiyat of Spike Schudy\".",
  "It's \"War and Peace\" (unabridged, very small print).",
  "A willing, ripe tomato bemoans your inability to digest fruit.",
  "A robot comedian. You feel amused.",
  "It's KITT, the talking car.",
  "Here's Pete Peterson. His batteries seem to have long gone dead.",
  "\"Blup, blup, blup\", says the mud pot.",
  "More grist for the mill.",
  "Grind 'em up, spit 'em out, they're twigs.",
  "The boom box cranks out an old Ethel Merman tune.",
  "It's \"Finding kitten\", published by O'Reilly and Associates.",
  "Pumpkin pie spice.",
  "It's the Bass-Matic '76! Mmm, that's good bass!",
  "\"Lend us a fiver 'til Thursday\", pleas Andy Capp.",
  "It's a tape of '70s rock. All original hits! All original artists!",
  "You've found the fabled America Online disk graveyard!",
  "Empty jewelboxes litter the landscape.",
  "It's the astounding meta-object.",
  "Ed McMahon stands here, lost in thought. Seeing you, he bellows, \"YES SIR!\"",
  "...thingy???",
  "It's 1000 secrets the government doesn't want you to know!",
  "The letters O and R.",
  "A magical... magic thing.",
  "It's a moment of silence.",
  "It's Sirhan-Sirhan, looking guilty.",
  "It's \"Chicken Soup for the Kitten-seeking Soulless Robot.\"",
  "It is a set of wind-up chatter teeth.",
  "It is a cloud shaped like an ox.",
  "You see a snowflake here, melting slowly.",
  "It's a big block of ice. Something seems to be frozen inside it.",
  "Vladimir Lenin's casket rests here.",
  "It's a copy of \"Zen and The Art of Robot Maintenance\".",
  "This invisible box contains a pantomime horse.",
  "A mason jar lies here open. It's label reads: \"do not open!\".",
  "A train of thought chugs through here.",
  "This jar of pickles expired in 1957.",
  "Someone's identity disk lies here.",
  "\"Yes!\" says the bit.",
  "\"No!\" says the bit.",
  "A dodecahedron bars your way.",
  "Mr. Hooper is here, surfing.",
  "It's a big smoking fish.",
  "You have new mail in /var/spool/robot",
  "Just a monitor with the blue element burnt out.",
  "A pile of coaxial plumbing lies here.",
  "It's a rotten old shoe.",
  "It's a hundred-dollar bill.",
  "It's a Dvorak keyboard.",
  "It's a cardboard box full of 8-tracks.",
  "Just a broken hard drive containg the archives of Nerth Pork.",
  "A broken metronome sits here, it's needle off to one side.",
  "A sign reads: \"Go home!\"",
  "A sign reads: \"No robots allowed!\"",
  "It's the handheld robotfindskitten game, by Tiger.",
  "This particular monstrosity appears to be ENIAC.",
  "This is a tasty-looking banana creme pie.",
  "A wireframe model of a hot dog rotates in space here.",
  "Just the empty husk of a locust.",
  "You disturb a murder of crows.",
  "It's a copy of the robotfindskitten EULA.",
  "It's Death.",
  "It's an autographed copy of \"Secondary Colors,\" by Bob Ross.",
  "It is a marzipan dreadnought that appears to have melted and stuck.",
  "It's a DVD of \"Crouching Monkey, Hidden Kitten\", region encoded for the moon.",
  "It's Kieran Hervold.  Damn dyslexia!",
  "A non-descript box of crackers.",
  "Carbonated Water, High Fructose Corn Syrup, Color, Phosphoric Acid, Flavors, Caffeine.",
  "\"Move along! Nothing to see here!\"",
  "It's the embalmed corpse of Vladimir Lenin.",
  "A coupon for one free steak-fish at your local family diner.",
  "A set of keys to a 2001 Rolls Royce. Worthless.",
  "A gravestone stands here.  \"Izchak Miller, ascended.\"",
  "Someone has written \"ad aerarium\" on the ground here.",
  "A large blue eye floats in midair.",
  "This appears to be a statue of Perseus.",
  "There is an opulent throne here.",
  "It's a squad of Keystone Kops.",
  "This seems to be junk mail addressed to the finder of the Eye of Larn.",
  "A wondrous and intricate golden amulet.  Too bad you have no neck.",
  "The swampy ground around you seems to stink with disease.",
  "An animate blob of acid.  Being metallic, you keep well away.",
  "It's a copy of Knuth with the chapter on kitten-search algorithms torn out.",
  "A crowd of people, and at the center, a popular misconception.",
  "It's a blind man. When you touch, he exclaims \"It's a kitten prospecting robot!\"",
  "It's a lost wallet. It's owner didn't have pets, so you discard it.",
  "This place is called Antarctica. There is no kitten here.",
  "It's a mousetrap, baited with soap.",
  "A book with \"Don't Panic\" in large friendly letters across the cover.",
  "A compendium of haiku about metals.",
  "A discredited cosmology, relic of a bygone era.",
  "A hollow voice says \"Plugh\".",
  "A knight who says \"Either I am an insane knave, or you will find kitten.\"",     
  "A neural net -- maybe it's trying to recognize kitten.",
  "A screwdriver.",
  "A statue of a girl holding a goose like the one in Gottingen, Germany.",
  "A tetradrachm dated \"42 B.C.\"",
  "A voice booms out \"Onward, kitten soldiers...\"",
  "An eminently forgettable zahir.",
  "Apparently, it's Edmund Burke.",
  "For a moment, you feel something in your hands, but it disappears!",
  "Here is a book about Robert Kennedy.",
  "Hey, robot, leave those lists alone.",
  "Ho hum.  Another synthetic a posteriori.",
  "It's Asimov's Laws of Robotics.  You feel a strange affinity for them.",
  "It's Bach's Mass in B-minor!",
  "It's a bug.",
  "It's a synthetic a priori truth!  Immanuel would be so pleased!",
  "It's the Tiki Room.",
  "Just some old play by a Czech playwright, and you can't read Czech.",
  "Kitten is the letter 'Q'.  Oh, wait, maybe not.",
  "Quidquid Latine dictum sit, kitten non est.",
  "Sutro Tower is visible at some distance through the fog.",
  "The Digital Millennium Copyright Act of 1998.",
  "The United States Court of Appeals for the Federal Circuit.",
  "The non-kitten item like this but with \"false\" and \"true\" switched is true.", 
  "The non-kitten item like this but with \"true\" and \"false\" switched is false.",
  "This is the chapter called \"A Map of the Cat?\" from Feynman's autobiography.",  
  "This is the forest primeval.",
  "Werner's \"Pocket Field Guide to Things That Are Not Kitten\".",
  "You found nettik, but that's backwards.",
  "You have found some zinc, but you must not stop here, for you must find kitten.", 
  "\"50 Years Among the Non-Kitten Items\", by Ann Droyd.",
  "\"Robot may not injure kitten, or, through inaction, ...\"",
  "\"Address Allocation for Private Internets\" by Yakov Rekhter et al.",
  "\"Mail Routing and the Domain System\" by Craig Partridge.",
  "\"The Theory and Practice of Oligarchical Collectivism\" by Emmanuel Goldstein.", 
  "\"201 Kitten Verbs, Fully Conjugated\".  You look for \"find\".",
  "A card shark sits here, practicing his Faro shuffle.  He ignores you.",
  "A copy of DeCSS.  They're a dime a dozen these days.",
  "A demonic voice proclaims \"There is no kitten, only Zuul\".  You flee.",
  "A lotus.  You make an interesting pair.",
  "A milk carton, with a black and white picture of kitten on the side.",
  "Any ordinary robot could see from a mile away that this wasn't kitten.",
  "A stegosaurus, escaped from the stegosaurusfindsrobot game.  It finds you.",
  "Baling wire and chewing gum.",
  "Chewing gum and baling wire.",
  "Here is no kitten but only rock, rock and no kitten and the sandy road.",
  "Hey, I bet you thought this was kitten.",
  "It is an ancient mariner, and he stoppeth one of three.",
  "It pleases you to be kind to what appears to be kitten -- but it's not!",
  "It's a blatant plug for Ogg Vorbis, http://www.vorbis.com/",
  "It's a business plan for a new startup, kitten.net.",
  "It's a revised business plan for a new startup, my.kitten.net.",
  "It's a square.",
  "It seems to be a copy of \"A Tail of Two Kitties\".",
  "It's the Donation of Constantine!",
  "It's this message, nothing more.",
  "Lysine, an essential amino acid.  Well, maybe not for robots.",
  "No kitten here.",
  "The score for a Czech composer's \"Kitten-Finding Symphony in C\".",
  "This looks like Bradley's \"Appearance and Reality\", but it's really not.",
  "This non-kitten item no verb.",
  "You feel strangely unfulfilled.",
  "You hit the non-kitten item.  The non-kitten item fails to yowl.",
  "You suddenly yearn for your distant homeland.",
  "You've found the snows of yesteryear!  So that's where they all went to.",
  "Approaching.  One car.  J.  Followed by.  Two car.  M, M.  In five. Minutes.",
  "Free Jon Johansen!",
  "Free Dmitry Sklyarov!",
  "One person shouts \"What do we want?\" The crowd answers \"Free Dmitry!\"",
  "Judith Platt insults librarians.",
  "This map is not the territory.",
  "\"Go back to Libraria!\", says Pat Schroeder.",
  "This is a porcelain kitten-counter.  0, 0, 0, 0, 0...",
  "An old bootable business card, unfortunately cracked down the middle.",
  "A kitten sink, for washing kitten (if only kitten liked water).",
  "A kitten source (to match the kitten sink).",
  "If it's one thing, it's not another.",
  "If it's not one thing, it's another.",
  "A caboodle.",
  "A grin.",
  "A hedgehog.  It looks like it knows something important.",
  "You've found... Oh wait, that's just a cat.",
  "Robot should not be touching that.",
  "Air Guitar!!!  NA na NA na!!",
  "An aromatherapy candle burns with healing light.",
  "You find a bright shiny penny.",
  "It's a free Jon Johansen!",
  "It's a free Dmitry Sklyarov!",
  "The rothe hits!  The rothe hits!",
  "It's an Internet chain letter about sodium laureth sulfate.",
  "Ed Witten sits here, pondering string theory.",
  "Something is written here in the dust.  You read: \"rJbotf ndQkttten\".",
  "We wish you a merry kitten, and a happy New Year!",
  "Run away!  Run away!",
  "You can see right through this copy of Brin\'s \"Transparent Society\".",
  "This copy of \"Steal This Book\" has been stolen from a bookstore.",
  "It's Roya Naini.",
  "This kit is the fourteenth in a series of kits named with Roman letters.",
  "This is the tenth key you've found so far.",
  "You find a fraud scheme in which loans are used as security for other loans.",
  "It's the phrase \"and her\", written in ancient Greek.",
  "It's the author of \"Randomness and Mathematical Proof\".",
  "It's the crusty exoskeleton of an arthropod!",
  "It's Emporer Shaddam the 4th's planet!",
  "It's the triangle leg adjacent to an angle divided by the leg opposite it.",
  "It's a bottle of nail polish remover.",
  "You found netkit! Way to go, robot!",
  "It's the ASCII Floating Head of Seth David Schoen!",
  "A frosted pink party-cake, half eaten.",
  "A bitchin' homemade tesla coil.",
  "Conan O'Brian, sans jawbone.",
  "It's either a mirror, or another soulless kitten-seeking robot.",
  "Preoccupation with finding kitten prevents you from investigating further.",
  "Fonzie sits here, mumbling incoherently about a shark and a pair of waterskis.",
  "The ghost of your dance instructor, his face a paper-white mask of evil.",
  "A bag of groceries taken off the shelf before the expiration date.",
  "A book: Feng Shui, Zen: the art of randomly arranging items that are not kitten.",
  "This might be the fountain of youth, but you'll never know.",
  "Tigerbot Hesh.",
  "Stimutacs.",
  "A canister of pressurized whipped cream, sans whipped cream.",
  "The non-kitten item bites!",
  "A chain hanging from two posts reminds you of the Gateway Arch.",
  "A mathematician calculates the halting probability of a Turing machine.",
  "A number of short theatrical productions are indexed 1, 2, 3, ... n.",
  "A technical university in Australia.",
  "It is -- I just feel something wonderful is about to happen.",
  "It's a Cat 5 cable.",
  "It's a U.S. president.",
  "It's a piece of cloth used to cover a stage in between performances.",
  "The ionosphere seems charged with meaning.",
  "This tomography is like, hella axial, man!",
  "It's your favorite game -- robotfindscatan!",
  "Just a man selling an albatross.",
  "The intermission from a 1930s silent movie.",
  "It's an inverted billiard ball!",
  "The spectre of Sherlock Holmes wills you onwards.",
};

#define TRUE true
#define FALSE false

#define RFK_VERSION "v1.4142135.406"

/* Button definitions stolen from maze.c */
#if (CONFIG_KEYPAD == IPOD_4G_PAD) || \
    (CONFIG_KEYPAD == IPOD_3G_PAD) || \
    (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#   undef __PLUGINLIB_ACTIONS_H__
#   define RFK_QUIT     (BUTTON_SELECT | BUTTON_MENU)
#   define RFK_RIGHT    BUTTON_RIGHT
#   define RFK_LEFT     BUTTON_LEFT
#   define RFK_UP       BUTTON_MENU
#   define RFK_DOWN     BUTTON_PLAY
#   define RFK_RRIGHT   (BUTTON_RIGHT | BUTTON_REPEAT)
#   define RFK_RLEFT    (BUTTON_LEFT | BUTTON_REPEAT)
#   define RFK_RUP      (BUTTON_MENU | BUTTON_REPEAT)
#   define RFK_RDOWN    (BUTTON_PLAY | BUTTON_REPEAT)

#else
#   define RFK_QUIT     PLA_QUIT
#   define RFK_RIGHT    PLA_RIGHT
#   define RFK_LEFT     PLA_LEFT
#   define RFK_UP       PLA_UP
#   define RFK_DOWN     PLA_DOWN
#   define RFK_RRIGHT   PLA_RIGHT_REPEAT
#   define RFK_RLEFT    PLA_LEFT_REPEAT
#   define RFK_RUP      PLA_UP_REPEAT
#   define RFK_RDOWN    PLA_DOWN_REPEAT

#endif
/*Constants for our internal representation of the screen.*/
#define EMPTY -1
#define ROBOT 0
#define KITTEN 1

/*Screen dimensions.*/
#define X_MIN 0
#define X_MAX ((LCD_WIDTH/SYSFONT_WIDTH) - 1)
#define Y_MIN 3
#define Y_MAX ((LCD_HEIGHT/SYSFONT_HEIGHT) - 1)

/* Colours used */
#if LCD_DEPTH >= 16
#define NUM_COLORS 6
#define ROBOT_COLOR LCD_DARKGRAY
const unsigned colors[NUM_COLORS] = {
    LCD_RGBPACK(255, 255, 0), /* Yellow */
    LCD_RGBPACK(0, 255, 255), /* Cyan */
    LCD_RGBPACK(255, 0, 255), /* Purple */
    LCD_RGBPACK(0, 0, 255), /* Blue */
    LCD_RGBPACK(255, 0, 0), /* Red */
    LCD_RGBPACK(0, 255, 0), /* Green */
};
#elif LCD_DEPTH == 2
#define NUM_COLORS 3
#define ROBOT_COLOR LCD_DARKGRAY
const unsigned colors[NUM_COLORS] = {
    LCD_LIGHTGRAY,
    LCD_DARKGRAY,
    LCD_BLACK,
};
#elif LCD_DEPTH == 1
#define NUM_COLORS 1
#define ROBOT_COLOR 0
const unsigned colors[NUM_COLORS] = {
    0,
};
#endif /* HAVE_LCD_COLOR */

/*Macros for generating numbers in different ranges*/
#define randx() (rb->rand() % X_MAX) + 1
#define randy() (rb->rand() % (Y_MAX-Y_MIN+1))+Y_MIN /*I'm feeling randy()!*/
#define randchar() rb->rand() % (126-'!'+1)+'!';
#define randcolor() rb->rand() % NUM_COLORS
#define randbold() (rb->rand() % 2 ? TRUE:FALSE)

/*Row constants for the animation*/
#define ADV_ROW 1
#define ANIMATION_MEET (X_MAX/3)*2
#define ANIMATION_LENGTH 4

/*This struct contains all the information we need to display an object
  on the screen*/
typedef struct
{
  short x;
  short y;
  int color;
  bool bold;
  char character;
} screen_object;

/*
 *Function definitions
 */

/*Initialization and setup functions*/
static void initialize_arrays(void);
static void initialize_robot(void);
static void initialize_kitten(void);
static void initialize_bogus(void);
static void initialize_screen(void);
static void instructions(void);
static void finish(int sig);

/*Game functions*/
static void play_game(void);
static void process_input(int);

/*Helper functions*/
static void pause(void);
static int validchar(char);
static void play_animation(int);

/*Global variables. Bite me, it's fun.*/
screen_object robot;
screen_object kitten;

#if X_MAX*Y_MAX < 200
#define NUM_BOGUS 15
#else
#define NUM_BOGUS 20
#endif
screen_object bogus[NUM_BOGUS];
unsigned short bogus_messages[NUM_BOGUS];
bool used_messages[MESSAGES];

bool exit_rfk;

/* This array contains our internal representation of the screen. The
 array is bigger than it needs to be, as we don't need to keep track
 of the first few rows of the screen. But that requires making an
 offset function and using that everywhere. So not right now. */
int screen[X_MAX + 1][Y_MAX + 1];

/* here is a global api struct pointer. while not strictly necessary,
   it's nice not to have to pass the api pointer in all function calls
   in the plugin */
static const struct plugin_api* rb;

/******************************************************************************
 *
 * Begin meaty routines that do the dirty work.
 *
 *****************************************************************************/

MEM_FUNCTION_WRAPPERS(rb) 

static void drawchar(int x, int y, char c)
{
  char str[2];
  rb->snprintf(str, sizeof(str), "%c", c);
  rb->lcd_putsxy(x*SYSFONT_WIDTH, y*SYSFONT_HEIGHT, str);
}

static void draw(screen_object o)
{
#if LCD_DEPTH > 1
  unsigned oldforeground;
  oldforeground = rb->lcd_get_foreground();
  rb->lcd_set_foreground(o.color);
  drawchar(o.x, o.y, o.character);
  rb->lcd_set_foreground(oldforeground);
#else
  drawchar(o.x, o.y, o.character);
#endif
}

static void message(char * str)
{
  rb->lcd_puts_scroll(0, ADV_ROW, str);
}

static void refresh(void)
{
  rb->lcd_update();
}

/*
 *play_game waits in a loop getting input and sending it to process_input
 */
static void play_game()
{
  int old_x = robot.x;
  int old_y = robot.y;
  int input = 0; /* Not sure what a reasonable initial value is */
#ifdef __PLUGINLIB_ACTIONS_H__
  const struct button_mapping *plugin_contexts[] = {generic_directions, generic_actions};
#endif

  while (input != RFK_QUIT && exit_rfk == false)
    {
      process_input(input);
      
      /*Redraw robot, where applicable. We're your station, robot.*/
      if (!(old_x == robot.x && old_y == robot.y))
    {
      /*Get rid of the old robot*/
    drawchar(old_x, old_y, ' ');
      screen[old_x][old_y] = EMPTY;
      
      /*Meet the new robot, same as the old robot.*/
      draw(robot);
      refresh();
      screen[robot.x][robot.y] = ROBOT;
      
      old_x = robot.x;
      old_y = robot.y;
    }
#ifdef __PLUGINLIB_ACTIONS_H__
      input = pluginlib_getaction(rb, TIMEOUT_BLOCK, plugin_contexts, 2);
#else
      input = rb->button_get(true);
#endif
    } 
  message("Bye!");
  refresh();
}

/*
 *Given the keyboard input, process_input interprets it in terms of moving,
 *touching objects, etc.
 */
static void process_input(int input)
{
  int check_x = robot.x;
  int check_y = robot.y;

  switch (input)
    {
    case RFK_UP:
    case RFK_RUP:
      check_y--;
      break;
    case RFK_DOWN:
    case RFK_RDOWN:
      check_y++;
      break;
    case RFK_LEFT:
    case RFK_RLEFT:
      check_x--;
      break;
    case RFK_RIGHT:
    case RFK_RRIGHT:
      check_x++;
      break;
    }
  
  /*Check for going off the edge of the screen.*/
  if (check_y < Y_MIN || check_y > Y_MAX || check_x < X_MIN || check_x > X_MAX)
    {
      return; /*Do nothing.*/
    }

  /*
   * Clear textline
   * disabled because it breaks the scrolling for some reason
   */
  /* rb->lcd_puts_scroll(0, ADV_ROW, " "); */
  
  /*Check for collision*/
  if (screen[check_x][check_y] != EMPTY)
    {
      switch (screen[check_x][check_y])
      {
        case ROBOT:
              /*We didn't move, or we're stuck in a
                time warp or something.*/
          break;
        case KITTEN: /*Found it!*/
          play_animation(input);
          /* Wait for the user to click something */
          pause();
          break;
        default: /*We hit a bogus object; print its message.*/
          message(messages[bogus_messages[screen[check_x][check_y]-2]]);
          refresh();
          break;
      }
      return;
    }

  /*Otherwise, move the robot.*/
  robot.x = check_x;
  robot.y = check_y;
}

/*finish is called upon signal or progam exit*/
static void finish(int sig)
{
  (void)sig;
  exit_rfk = true;
}

/******************************************************************************
 *
 * Begin helper routines
 *
 *****************************************************************************/

static void pause()
{
  int button;
  rb->lcd_update();
  do
    button = rb->button_get(true);
  while( ( button == BUTTON_NONE )
      || ( button & (BUTTON_REL|BUTTON_REPEAT) ) );
}

static int validchar(char a)
{
  switch(a)
    {
    case '#':
    case ' ':   
    case 127:
      return 0;
    }
  return 1;
}

static void play_animation(int input)
{
  int counter;
  screen_object left;
  screen_object right;
  /*The grand cinema scene.*/
  rb->lcd_puts_scroll(0, ADV_ROW, " ");

  if (input == RFK_RIGHT || input == RFK_DOWN || input == RFK_RRIGHT || input == RFK_RDOWN) {
    left = robot;
    right = kitten;
  }
  else {
    left = kitten;
    right = robot;
  }
  left.y = ADV_ROW;
  right.y = ADV_ROW;
  left.x = ANIMATION_MEET - ANIMATION_LENGTH - 1;
  right.x = ANIMATION_MEET + ANIMATION_LENGTH;

  for (counter = ANIMATION_LENGTH; counter > 0; counter--)
  {
    left.x++;
    right.x--;
    /* Clear the previous position (empty the first time) */
    drawchar(left.x - 1, left.y, ' ');
    drawchar(right.x + 1, right.y, ' ');
    draw(left);
    draw(right);
    refresh();
    rb->sleep(HZ);
  }

  message("You found kitten! Way to go, robot!");
  refresh();
  finish(0);
}

/******************************************************************************
 *
 * Begin initialization routines (called before play begins).
 *
 *****************************************************************************/

static void instructions()
{
#define MARGIN 2
  int y = MARGIN, space_w, width, height;
  unsigned short x = MARGIN, i = 0;
#define WORDS (sizeof instructions / sizeof (char*))
  static char* instructions[] = {
#if 0
    /* Not sure if we want to include this? */
    "robotfindskitten", RFK_VERSION, "", "",
    "By", "the", "illustrious", "Leonard", "Richardson", "(C)", "1997,", "2000", "",
    "Written", "originally", "for", "the", "Nerth", "Pork", "robotfindskitten", "contest", "", "",
#endif
    "In", "this", "game", "you", "are", "robot", "(#).", "Your", "job", "is", "to", "find", "kitten.", "This", "task", "is", "complicated", "by", "the", "existence", "of", "various", "things", "which", "are", "not", "kitten.", "Robot", "must", "touch", "items", "to", "determine", "if", "they", "are", "kitten", "or", "not.", "",
    "The", "game", "ends", "when", "robotfindskitten.", "", "",
    "Press", "any", "key", "to", "start",
  };
  rb->lcd_clear_display();
  rb->lcd_getstringsize(" ", &space_w, &height);
  for (i = 0; i < WORDS; i++) {
    rb->lcd_getstringsize(instructions[i], &width, NULL);
    /* Skip to next line if the current one can't fit the word */
    if (x + width > LCD_WIDTH - MARGIN) {
      x = MARGIN;
      y += height;
    }
    /* .. or if the word is the empty string */
    if (rb->strcmp(instructions[i], "") == 0) {
      x = MARGIN;
      y += height;
      continue;
    }
    /* We filled the screen */
    if (y + height > LCD_HEIGHT - MARGIN) {
      y = MARGIN;
      pause();
      rb->lcd_clear_display();
    }
    rb->lcd_putsxy(x, y, instructions[i]);
    x += width + space_w;
  }
  pause();
}

static void initialize_arrays()
{
  unsigned int counter, counter2;
  screen_object empty;

  /*Initialize the empty object.*/
  empty.x = -1;
  empty.y = -1;
#if LCD_DEPTH > 1
  empty.color = LCD_BLACK;
#else
  empty.color = 0;
#endif
  empty.bold = FALSE;
  empty.character = ' ';
  
  for (counter = 0; counter <= X_MAX; counter++)
  {
    for (counter2 = 0; counter2 <= Y_MAX; counter2++)
    {
      screen[counter][counter2] = EMPTY;
    }
  }
  
  /*Initialize the other arrays.*/
  for (counter = 0; counter < MESSAGES; counter++)
  {
    used_messages[counter] = false;
  }
  for (counter = 0; counter < NUM_BOGUS; counter++)
  {
    bogus_messages[counter] = 0;
    bogus[counter] = empty;
  }
}

/*initialize_robot initializes robot.*/
static void initialize_robot()
{
  /*Assign a position to the player.*/
  robot.x = randx();
  robot.y = randy();

  robot.character = '#';
  robot.color = ROBOT_COLOR;
  robot.bold = FALSE;
  screen[robot.x][robot.y] = ROBOT;
}

/*initialize kitten, well, initializes kitten.*/
static void initialize_kitten()
{
  /*Assign the kitten a unique position.*/
  do
    {
      kitten.x = randx();
      kitten.y = randy();
    } while (screen[kitten.x][kitten.y] != EMPTY);
  
  /*Assign the kitten a character and a color.*/
  do {
    kitten.character = randchar();
  } while (!(validchar(kitten.character))); 
  screen[kitten.x][kitten.y] = KITTEN;

  kitten.color = colors[randcolor()];
  kitten.bold = randbold();
}

/*initialize_bogus initializes all non-kitten objects to be used in this run.*/
static void initialize_bogus()
{
  int counter, index;
  for (counter = 0; counter < NUM_BOGUS; counter++)
    {
      /*Give it a color.*/
      bogus[counter].color = colors[randcolor()];
      bogus[counter].bold = randbold();
      
      /*Give it a character.*/
      do {
    bogus[counter].character = randchar();
      } while (!(validchar(bogus[counter].character))); 
      
      /*Give it a position.*/
      do
    {
      bogus[counter].x = randx();
      bogus[counter].y = randy();
    } while (screen[bogus[counter].x][bogus[counter].y] != EMPTY);

      screen[bogus[counter].x][bogus[counter].y] = counter+2;
      
      /*Find a message for this object.*/
      do {
        index = rb->rand() % MESSAGES;
      } while (used_messages[index] != false);
      bogus_messages[counter] = index;
      used_messages[index] = true;
    }

}

/*initialize_screen paints the screen.*/
static void initialize_screen()
{
  int counter;
  char buf[40];

  /*
   *Print the status portion of the screen.
   */
  rb->lcd_clear_display();
  rb->lcd_setfont(FONT_SYSFIXED);
  rb->snprintf(buf, sizeof(buf), "robotfindskitten %s", RFK_VERSION);
  rb->lcd_puts_scroll(0, 0, buf);
  refresh();
  
  /*Draw a line across the screen.*/
  for (counter = X_MIN; counter <= X_MAX + 1; counter++)
    {
      drawchar(counter, ADV_ROW+1, '_');
    }

  /*
   *Draw all the objects on the playing field.
   */
  for (counter = 0; counter < NUM_BOGUS; counter++)
    {
      draw(bogus[counter]);
    }

  draw(kitten);
  draw(robot);

  refresh();

}

/* this is the plugin entry point */
enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
  (void)parameter;
  rb = api;

  exit_rfk = false;

  rb->srand(*rb->current_tick);

  initialize_arrays();

  /*
   * Now we initialize the various game objects.
   */
  initialize_robot();
  initialize_kitten();
  initialize_bogus();

  /*
   * Set up white-on-black screen on color targets
   */
#if LCD_DEPTH >= 16
  rb->lcd_set_backdrop(NULL);
  rb->lcd_set_foreground(LCD_WHITE);
  rb->lcd_set_background(LCD_BLACK);
#endif

  /*
   * Run the game
   */
  instructions();  

  initialize_screen();

  play_game();

  rb->lcd_setfont(FONT_UI);
  return PLUGIN_OK;
}
