AI Controller Architecture
==========================

The AI controller plays a HAR the same way a human player does: every tick it produces controller
inputs, and it reacts to events sent by the HAR. The entry point is ``src/controller/ai_controller.c``.
Most of the logic lives in small modules under ``src/game/ai/``, and most tuning values are loaded
from files under ``resources/ai_config/``.

Controller Interface
--------------------

``ai_controller_create()`` allocates the AI state and installs three callbacks on the controller:

============  ======================  =========================================
Callback      Function                Purpose
============  ======================  =========================================
``poll_fun``  ``ai_controller_poll``  Called every tick to produce input events
``har_hook``  ``ai_har_event``        Called when the HAR fires a ``har_event``
``free_fun``  ``ai_controller_free``  Frees the AI state
============  ======================  =========================================

The rest of the game does not need to know it is talking to an AI.

Per-Tick Flow
-------------

.. graphviz::

   digraph ai_poll {
       rankdir=TB
       node [shape=box, fontname="monospace"]

       poll [label="ai_controller_poll()", shape=ellipse]
       active [label="Fighting and\nnot paused?", shape=diamond]
       block [label="Block HAR or\nprojectile"]
       selected [label="Finish selected move"]
       tactic [label="Continue queued tactic"]
       attack [label="Random attack"]
       movement [label="Movement"]
       queue [label="Maybe queue\na new tactic"]
       done [label="Return", shape=ellipse]

       poll -> active
       active -> done [label="no"]
       active -> block [label="yes"]
       block -> selected -> tactic -> attack -> movement -> queue -> done
   }

Each step returns early once it has produced input, so only one of them acts per tick:

1. **Blocking**: block an incoming attack or projectile if the pilot decides to.
2. **Selected move**: keep feeding inputs for a move that is part way through.
3. **Queued tactic**: advance the current tactic (see below).
4. **Random attack**: occasionally attack, scaled by difficulty and range.
5. **Movement**: walk, jump or hold position based on pilot preferences.
6. **New tactic**: if nothing is queued, consider a new tactic for the next tick.

The AI does nothing while the game is paused or while the arena is not in the fighting state.

Tactics
-------

A tactic is a short plan made of a movement type and an attack type. For example ``GRAB`` closes
the distance and then attempts a throw. Tactics are defined in ``ai_tactic_engine.h``:

=======  ==================================
Tactic   Purpose
=======  ==================================
ESCAPE   Get away from the enemy
TURTLE   Block attacks
GRAB     Close in and throw
SPAM     Repeat the same attack
SHOOT    Use a projectile
TRIP     Trip the enemy
QUICK    Quick attack
CLOSE    Close the distance and attack
FLY      Jump at the enemy
PUSH     Power moves to push the enemy back
COUNTER  Block, then attack
=======  ==================================

A tactic is queued either from the per-tick flow or in response to a HAR event. Before queuing,
``ai_tactic_likes_it()`` checks whether the pilot, difficulty and current situation suit the tactic.
A queued tactic runs until it lands an attack, runs out of time, or is cancelled by an event.

Individual tactics can be switched off in ``resources/ai_config/tactics.json``.

HAR Events
----------

The HAR reports what happens to it through ``har_event`` (see ``src/game/objects/har.h``).
``ai_har_event()`` handles each event in two steps:

1. ``ai_event_check_cancel_tactic()`` decides whether the queued tactic should be dropped or extended.
2. A handler in ``ai_event.c`` updates learning data and may queue a follow-up tactic.

Some examples:

==============================  ===========================================================
Event                           AI response
==============================  ===========================================================
``HAR_EVENT_LAND_HIT``          Record the hit distance for the move used
``HAR_EVENT_TAKE_HIT``          Drop exposed tactics such as CLOSE and FLY
``HAR_EVENT_BLOCK``             Counter-attack, or pick a tactic to answer the attack
``HAR_EVENT_ENEMY_STUN``        Extend GRAB, CLOSE or TRIP, or queue one to punish the stun
``HAR_EVENT_ENEMY_HAZARD_HIT``  Queue a follow-up while the enemy recovers
``HAR_EVENT_HAZARD_PUNCHED``    The HAR punched the fire pit orb, so drop the queued tactic
==============================  ===========================================================

Modules
-------

=============================  =========================================================
File                           Responsibility
=============================  =========================================================
``ai_decision_engine.c``       Difficulty-scaled random rolls (``smart_usually()`` etc.)
``ai_utils.c``                 Shared checks such as enemy range and stun state
``ai_state.c``                 AI state setup and resets
``ai_movement.c``              Movement decisions
``ai_move_selector.c``         Scoring and choosing an attack move
``ai_tactic_engine.c``         Tactic choice, tactic config and hazard checks
``ai_har_skills.c``            Charge, push, trip and projectile moves from HAR config
``ai_event.c``                 HAR event handling
``ai_learning.c``              Learning from throws and projectiles, anti-spam cooldowns
``ai_config_loader.c``         Loads ``pilots.json``
``ai_skills_config_loader.c``  Loads ``hars/*.json``
``ai_core_config.c``           Loads ``ai_core.ini`` and the per-difficulty files
=============================  =========================================================

Configuration
-------------

All AI config lives in ``resources/ai_config/``:

============================  ======================================================
File                          Contents
============================  ======================================================
``pilots.json``               Pilot personalities: attack preferences, learning rate
``tactics.json``              Tactic definitions and on/off switches
``hars/<har>.json``           Charge, push and projectile moves for each HAR
``ai_core.ini``               Shared timing, chance and behaviour values
``ai_difficulty/<name>.ini``  Per-difficulty overrides of ``ai_core.ini``
============================  ======================================================

``pilots.json`` and ``hars/*.json`` can be overridden by mods through the mod manager.

Difficulty Settings
~~~~~~~~~~~~~~~~~~~

``ai_core.ini`` is loaded first, then the file for the current difficulty (for example
``ai_difficulty/rookie.ini``) overrides any values it sets. Besides timing and chance values, a few
keys switch whole behaviours on or off, so they can be tuned per difficulty without code changes:

=================================  ===========================================
Key                                Effect when set to 1
=================================  ===========================================
``aggressive_tactics``             Allow advanced tactics such as COUNTER
``fire_orb_exploitation_enabled``  Use the fire pit orb on purpose (see below)
``stun_capitalize_enabled``        Follow up on a stunned enemy
=================================  ===========================================

HAR Move Conditions
~~~~~~~~~~~~~~~~~~~

Each move in ``hars/*.json`` has an input sequence, a range, and an optional list of conditions.
Moves are checked in file order and the first one whose range and conditions all pass is used.

.. code-block:: json

   {
     "name": "shadow_projectile_punch",
     "sequence": ["D", "DB", "B", "P"],
     "range_min": "ANY",
     "conditions": ["roll_d10", "high_difficulty"]
   }

Supported conditions are ``high_difficulty``, ``special_preferred``, ``low_preferred``,
``jump_preferred``, ``roll_d2``, ``roll_d3``, ``roll_d4``, ``roll_d10``, ``roll_d20``,
``enemy_stunned`` and ``enemy_not_stunned``.

Arena Hazards
-------------

The tactic engine checks for a few hazard situations and makes matching tactics more likely:

- **Fire pit orb**: if the orb lines up with the enemy, prefer CLOSE and QUICK. If the orb is directly
  overhead, prefer FLY. Controlled by ``fire_orb_exploitation_enabled``.
- **Wall pressure**: if the enemy is near a wall hazard, prefer pressure tactics.
- **Spikes**: if the AI is near a spike, prefer getting away.

When a HAR attack hits the orb, ``har_collide_with_hazard()`` fires ``HAR_EVENT_HAZARD_PUNCHED``.
The event says whether it was a ground punch or a jump punch.

Testing and Debugging
---------------------

AI unit tests live in ``testing/ai/``. Most of them only run when
``OPENOMF_RUN_DETERMINISTIC_TESTS=1`` is set, because they are meant for active AI tuning:

.. code-block:: bash

   cmake -DUSE_TESTS=On -S . -B build
   cmake --build build
   OPENOMF_RUN_DETERMINISTIC_TESTS=1 ./build/openomf_test_main

AI decisions are logged under the ``ai`` and ``tactic`` log modules. To see them in game, run
``openomf --log-level DEBUG --log-modules ai,tactic``.
