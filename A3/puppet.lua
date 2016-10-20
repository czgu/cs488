-- puppet.lua
-- A simplified puppet without posable joints, but that
-- looks roughly humanoid.

rootnode = gr.node('root')
rootnode:rotate('y', -20.0)
rootnode:scale( 0.25, 0.25, 0.25 )
rootnode:translate(0.0, 0.0, -1.0)

red = gr.material({1.0, 0.0, 0.0}, {0.1, 0.1, 0.1}, 10)
blue = gr.material({0.0, 0.0, 1.0}, {0.1, 0.1, 0.1}, 10)
green = gr.material({0.0, 1.0, 0.0}, {0.1, 0.1, 0.1}, 10)
white = gr.material({1.0, 1.0, 1.0}, {0.1, 0.1, 0.1}, 10)
yellow = gr.material({1.0, 1.0, 0.0}, {0.1, 0.1, 0.1}, 10)
purple = gr.material({1.0, 0.0, 1.0}, {0.1, 0.1, 0.1}, 10)

function makeLimb()
    shoulderJoint = gr.joint('shoulderJoint', {180, 360, 360}, {0, 0, 0})

    shoulder = gr.mesh('sphere', 'shoulder')
    shoulder:scale(0.1, 0.1, 0.1)
    shoulder:set_material(blue)
    shoulderJoint:add_child(shoulder)

    upper = gr.mesh('cube', 'UpperLimb')
    upper:scale(0.1, 0.4, 0.1)
    upper:translate(0, -0.2, 0)
    upper:set_material(red)
    shoulderJoint:add_child(upper)

    elbowJoint = gr.joint('elbowJoint', {200, 360, 360}, {0, 0, 0})
    elbowJoint:translate(0, -0.4, 0);

    elbow= gr.mesh('sphere', 'elbow')
    elbow:scale(0.1, 0.1, 0.1)
    elbow:set_material(blue)
    elbowJoint:add_child(elbow)

    lower = gr.mesh('cube', 'LowerLimb')
    lower:scale(0.1, 0.4, 0.1)
    lower:translate(0, -0.2, 0);
    lower:set_material(green)
    elbowJoint:add_child(lower)

    wristJoint = gr.joint('wristJoint', {200, 360, 360}, {0,0,0})
    wristJoint:translate(0, -0.4, 0);

    hand = gr.mesh('cube', 'hand')
    hand:scale(0.15, 0.15, 0.05);
    hand:set_material(red)
    hand:translate(0, -0.075, 0);
    wristJoint:add_child(hand)

    elbowJoint:add_child(wristJoint)

    shoulderJoint:add_child(elbowJoint)
    return shoulderJoint
end

function makeShoulder()
    centreJoint = gr.joint('centreJoint', {0, 0, 360}, {0, 0, 0});

    shoulder = gr.mesh('sphere', 'Shoulder');
    shoulder:scale(0.6, 0.25, 0.25);
    shoulder:set_material(yellow);
    centreJoint:add_child(shoulder)

    leftArm = makeLimb()
    leftArm:translate(-0.6, 0.0, 0.0)
    centreJoint:add_child(leftArm)

    rightArm = makeLimb()
    rightArm:translate(0.6, 0.0, 0.0)
    centreJoint:add_child(rightArm)

    return centreJoint
end

function makeHead()
    neckLowJoint = gr.joint('neckLowJoint', {0, 0, 360}, {0, 0, 0});

    neck = gr.mesh('sphere', 'neck')
    neck:scale(0.10, 0.15, 0.10)
    neck:translate(0.0, 0.075, 0.0)
    neck:set_material(blue)
    neckLowJoint:add_child(neck)

    neckHighJoint = gr.joint('neckHighJoint', {0, 0, 360}, {0, 0, 360});
    neckHighJoint:translate(0.0, 0.15, 0.0);
    neckLowJoint:add_child(neckHighJoint)

    head = gr.mesh('sphere', 'head')
    neckHighJoint:add_child(head)
    head:scale(0.3, 0.3, 0.3)
    head:translate(0.0, 0.2, 0.0)
    head:set_material(white)

    ears = gr.mesh('sphere', 'ears')
    head:add_child(ears)
    ears:scale(1.6, 0.08, 0.08)
    ears:set_material(red)
    ears:set_material(blue)

    leftEye = gr.mesh('sphere', 'leftEye')
    head:add_child(leftEye)
    leftEye:scale(0.1, 0.1, 0.1)
    leftEye:translate(-0.25, 0.15, 1.0)
    leftEye:set_material(blue)

    rightEye = gr.mesh('sphere', 'rightEye')
    head:add_child(rightEye)
    rightEye:scale(0.1, 0.1, 0.1)
    rightEye:translate(0.25, 0.15, 1.0)
    rightEye:set_material(blue)

    return neckLowJoint
end

jointTorso = gr.joint('jointTorso', {0,0,0}, {0,0,0});
rootnode:add_child(jointTorso)

shoulder = makeShoulder()
shoulder:translate(0, 0.75, 0);
jointTorso:add_child(shoulder)

hip = makeShoulder();
hip:translate(0, -0.75, 0);
jointTorso:add_child(hip)

head = makeHead();
head:translate(0, 1.0, 0)
jointTorso:add_child(head)

torso = gr.mesh('cube', 'torso')
torso:set_material(white)
torso:scale(0.75,1.25,0.5);
jointTorso:add_child(torso)

return rootnode
