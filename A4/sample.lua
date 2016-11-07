-- test for hierarchical ray-tracers.
-- Thomas Pflaum 1996

gold = gr.material({0.9, 0.8, 0.4}, {0.8, 0.8, 0.4}, 25)
grass = gr.material({0.1, 0.7, 0.1}, {0.9, 0.9, 0.9}, 20)
blue = gr.material({0.7, 0.6, 1}, {0.5, 0.4, 0.8}, 25)

red = gr.material({1.0, 0.0, 0.0}, {0.1, 0.1, 0.1}, 10)
green = gr.material({0.0, 1.0, 0.0}, {0.1, 0.1, 0.1}, 10)
white = gr.material({1.0, 1.0, 1.0}, {0.1, 0.1, 0.1}, 10)
yellow = gr.material({1.0, 1.0, 0.0}, {0.1, 0.1, 0.1}, 10)
purple = gr.material({1.0, 0.0, 1.0}, {0.1, 0.1, 0.1}, 10)
stone = gr.material({0.8, 0.7, 0.7}, {0.0, 0.0, 0.0}, 0)
hide = gr.material({0.84, 0.6, 0.53}, {0.3, 0.3, 0.3}, 50)

-- #############################################
-- Read in the cow model from a separate file.
-- #############################################

cow_poly = gr.mesh('cow', 'cow.obj')
factor = 2.0/(2.76+3.637)

cow_poly:set_material(hide)

hide = gr.material({0.84, 0.6, 0.53}, {0.3, 0.3, 0.3}, 20)
cow_poly:translate(0.0, 3.637, 0.0)
cow_poly:scale(factor, factor, factor)
cow_poly:translate(0.0, -1.0, 0.0)



 -- LOAD THE PUPPET
function makeLimb(isUpper)
    if isUpper then
        shoulderJoint = gr.joint('shoulderJoint', {180, 360, 360}, {0, 0, 0})
    else
        shoulderJoint = gr.joint('shoulderJoint', {270, 360, 360}, {0, 0, 0})
    end

    shoulder = gr.sphere('shoulder')
    shoulder:scale(0.1, 0.1, 0.1)
    shoulder:set_material(blue)
    shoulderJoint:add_child(shoulder)

    upper = gr.cube('UpperLimb')
    upper:scale(0.1, 0.4, 0.1)
    upper:translate(0, -0.2, 0)
    upper:set_material(red)
    shoulderJoint:add_child(upper)

    if isUpper then
        elbowJoint = gr.joint('elbowJoint', {-160, -80, 0}, {0, 0, 0})
    else
        elbowJoint = gr.joint('elbowJoint', {0, 0, 120}, {0, 0, 0})
    end

    elbowJoint:translate(0, -0.4, 0);

    elbow= gr.sphere('elbow')
    elbow:scale(0.1, 0.1, 0.1)
    elbow:set_material(blue)
    elbowJoint:add_child(elbow)

    lower = gr.cube('LowerLimb')
    lower:scale(0.1, 0.4, 0.1)
    lower:translate(0, -0.2, 0);
    lower:set_material(green)
    elbowJoint:add_child(lower)

    if isUpper then
        wristJoint = gr.joint('wristJoint', {-90, 0, 60}, {0,0,0})
    else -- Foot
        wristJoint = gr.joint('wristJoint', {-90, -90, 0}, {0,0,0})
    end

    wristJoint:translate(0, -0.4, 0);

    hand = gr.cube('hand')
    hand:scale(0.15, 0.15, 0.05);
    hand:set_material(red)
    hand:translate(0, -0.035, 0);
    wristJoint:add_child(hand)

    elbowJoint:add_child(wristJoint)

    shoulderJoint:add_child(elbowJoint)
    return shoulderJoint
end

function makeShoulder(isUpper)
    centreJoint = gr.joint('centreJoint', {0, 0, 0}, {-30, 0, 30});

    shoulder = gr.sphere('Shoulder');
    shoulder:scale(0.6, 0.25, 0.25);
    shoulder:set_material(yellow);
    centreJoint:add_child(shoulder)

    leftArm = makeLimb(isUpper)
    leftArm:translate(-0.6, 0.0, 0.0)
    centreJoint:add_child(leftArm)

    rightArm = makeLimb(isUpper)
    rightArm:translate(0.6, 0.0, 0.0)
    centreJoint:add_child(rightArm)

    return centreJoint
end

function makeHead()
    neckLowJoint = gr.joint('neckLowJoint', {0, 0, 90}, {-90, 0, 90});

    neck = gr.sphere('neck')
    neck:scale(0.10, 0.15, 0.10)
    neck:translate(0.0, 0.075, 0.0)
    neck:set_material(blue)
    neckLowJoint:add_child(neck)

    head = gr.sphere('head')
    neckLowJoint:add_child(head)
    head:scale(0.3, 0.3, 0.3)
    head:translate(0.0, 0.35, 0.0)
    head:set_material(white)

    ears = gr.sphere('ears')
    head:add_child(ears)
    ears:scale(1.6, 0.08, 0.08)
    ears:set_material(red)
    ears:set_material(blue)

    leftEye = gr.sphere('leftEye')
    head:add_child(leftEye)
    leftEye:scale(0.1, 0.1, 0.1)
    leftEye:translate(-0.25, 0.15, 1.0)
    leftEye:set_material(blue)

    rightEye = gr.sphere('rightEye')
    head:add_child(rightEye)
    rightEye:scale(0.1, 0.1, 0.1)
    rightEye:translate(0.25, 0.15, 1.0)
    rightEye:set_material(blue)

    return neckLowJoint
end

puppet = gr.node('puppet')

jointTorso = gr.joint('jointTorso', {0,0,0}, {0,0,0});
puppet:add_child(jointTorso)

shoulder = makeShoulder(true)
shoulder:translate(0, 0.75, 0);
jointTorso:add_child(shoulder)

hip = makeShoulder(false);
hip:translate(0, -0.75, 0);
jointTorso:add_child(hip)

head = makeHead();
head:translate(0, 1.0, 0)
jointTorso:add_child(head)

torso = gr.cube('torso')
torso:set_material(white)
torso:scale(0.65,1.25,0.4);
torso:translate(-0.35, -0.45, -0.2)
jointTorso:add_child(torso)



scene = gr.node('scene')
scene:rotate('X', 23)
scene:translate(6, -2, -15)

-- the arc
arc = gr.node('arc')
scene:add_child(arc)
arc:translate(0,0,-10)
arc:rotate('Y', 60)
p1 = gr.cube('p1')
arc:add_child(p1)
p1:set_material(gold)
p1:scale(0.8, 7, 0.8)
p1:translate(-2.4, 0, -0.4)

p2 = gr.cube('p2')
arc:add_child(p2)
p2:set_material(gold)
p2:scale(0.8, 7, 0.8)
p2:translate(1.6, 0, -0.4)

s = gr.sphere('s')
arc:add_child(s)
s:set_material(stone)
s:scale(4, 0.6, 0.6)
s:translate(0, 7, 0)

-- the floor
plane = gr.mesh( 'plane', 'Assets/plane.obj' )
scene:add_child(plane)
plane:set_material(grass)
plane:scale(30, 30, 30)

-- sphere
poly = gr.mesh( 'poly', 'Assets/dodeca.obj' )
scene:add_child(poly)
poly:translate(-2, 3.618034, 0)
poly:set_material(blue)

-- The lights
l1 = gr.light({200,200,400}, {0.8, 0.8, 0.8}, {1, 0, 0})
l2 = gr.light({0, 5, -20}, {0.4, 0.4, 0.8}, {1, 0, 0})

puppet:scale(2, 2, 2)
puppet:translate(-6, 3, -1)
scene:add_child(puppet)


cow_number = 1
for _, pt in pairs({
		      {{-7.5,1.3,0}, -60}}) do
   cow_instance = gr.node('cow' .. tostring(cow_number))
   scene:add_child(cow_instance)
   cow_instance:add_child(cow_poly)
   cow_instance:scale(1.4, 1.4, 1.4)
   cow_instance:rotate('Y', pt[2])
   cow_instance:translate(table.unpack(pt[1]))
   
   cow_number = cow_number + 1
end

gr.render(scene, 'sample.png', 500, 500, 
	  {0, 0, 0,}, {0, 0, -1}, {0, 1, 0}, 50,
	  {0.4, 0.4, 0.4}, {l1, l2})

