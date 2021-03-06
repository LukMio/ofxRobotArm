//Copyright (c) 2016, Daniel Moore, Madeline Gannon, and The Frank-Ratchye STUDIO for Creative Inquiry All rights reserved.

//--------------------------------------------------------------
//
//
// Basic Move Example
//
//
//--------------------------------------------------------------

//
// This example shows you how to:
//  1.  Connect ofxRobotArm to your robot via ethernet
//  2.  Move & reiorient the simulated robot (dragging ofxGizmo)
//  3.  Move & reiorient the real-time robot (use 'm' to enable real-time movement)
//
// Remeber to swap in your robot's ip address in robot.setup() [line 40]
// If you don't know your robot's ip address, you may not be set up yet ...
//  -   Refer to http://www.universal-robots.com/how-tos-and-faqs/how-to/ur-how-tos/ethernet-ip-guide-18712/
//      for a walk-thru to setup you ethernet connection
// See the ReadMe for more tutorial details


#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    
    ofSetFrameRate(60);
    ofSetVerticalSync(true);
    ofBackground(0);
    ofSetLogLevel(OF_LOG_VERBOSE);
    camUp = ofVec3f(0, 0, 1);
    
        ofEnableAlphaBlending();
    parameters.setup();
    robot.setup("192.168.1.3", parameters, true); // <-- change to your robot's ip address
    
    
    
   
    setupGUI();
    setupViewports();
    positionGUI();
    
    
    sim = 0;
    real = 1;
    
    handleViewportPresets('1');
}

void ofApp::exit(){
    robot.close();
    for(int i = 0 ; i < cams.size(); i++){
        if(cams[i]){
            delete cams[i];
            cams[i] = NULL;
        }
    }
}

//--------------------------------------------------------------
void ofApp::update(){
    robot.setTeachMode();
    moveTCP();
    robot.update();
    updateActiveCamera();
    
    if(parameters.bTeachMode){
        parameters.bMove = false;
        parameters.bCopy = true;
    }
    
    if(parameters.bMove){
        previewMode = moveColor;
        realMode = moveColor;
    }
    else if(parameters.bTeachMode){
        previewMode = freedriveColor;
        realMode = freedriveColor;
    }else{
        previewMode = stopColor;
        realMode = stopColor;
    }
}


//--------------------------------------------------------------
void ofApp::draw(){
    ofPushStyle();
    ofSetColor(255,160);
    ofDrawBitmapString("OF FPS "+ofToString(ofGetFrameRate()), 30, ofGetWindowHeight()-50);
    ofDrawBitmapString("Robot FPS "+ofToString(robot.robot.getThreadFPS()), 30, ofGetWindowHeight()-65);
    
    

    // show realtime robot
    cams[real]->begin(viewportReal);
    ofDrawAxis(100);
    tcpNode.draw();

    robot.draw(realMode);
    robot.drawPreview(ofFloatColor(previewMode.r, previewMode.g, previewMode.b, 0.20));
 
    robot.drawSafety(*cams[real]);
    cams[real]->end();
    
    // show simulation robot
    cams[sim]->begin(viewportSim);
    ofDrawAxis(100);
    tcpNode.draw();
    robot.drawPreview(previewMode);
    gizmo.draw(*cams[sim]);
    cams[sim]->end();
    
    
    
    drawGUI();
    ofPopStyle();
    
}

void ofApp::moveTCP(){
    
    // assign the target pose to the current robot pose
    if(parameters.bCopy){
        parameters.bCopy = false;
        
        parameters.bCopy = false;
        parameters.targetTCP.rotation = ofQuaternion(90, ofVec3f(0, 0, 1));
        parameters.targetTCP.rotation*=ofQuaternion(90, ofVec3f(1, 0, 0));
        
        // get the robot's position
        parameters.targetTCP.position = parameters.actualTCP.position;
        parameters.targetTCP.rotation*=parameters.actualTCP.rotation;
        
        tcpNode.setPosition(parameters.targetTCP.position*1000);
        tcpNode.setOrientation(parameters.targetTCP.rotation);
        gizmo.setNode(tcpNode);
        // update GUI params
        parameters.targetTCPPosition = parameters.targetTCP.position;
        parameters.targetTCPOrientation = ofVec4f(parameters.targetTCP.rotation.x(), parameters.targetTCP.rotation.y(), parameters.targetTCP.rotation.z(), parameters.targetTCP.rotation.w());
        
    }
    else{
        
    }
    
    // follow the gizmo's position and orientation
    if(parameters.bFollow){
        gizmo.apply(tcpNode);
        parameters.targetTCP.position.interpolate(tcpNode.getPosition()/1000.0, parameters.followLerp);
        parameters.targetTCP.rotation = tcpNode.getOrientationQuat();
        parameters.targetTCPOrientation = ofVec4f(parameters.targetTCP.rotation.x(), parameters.targetTCP.rotation.y(), parameters.targetTCP.rotation.z(), parameters.targetTCP.rotation.w());
    }
    
    if(parameters.bUseTimeline){
        robot.enableControlJointsExternally();
        vector<double> pose;
        for(int i = 0; i < jointNames.size(); i++){
            pose.push_back(timeline.getValue(jointNames[i]));
        }
        
        robot.update(pose);
    }else{
        robot.disableControlJointsExternally();
    }
}

void ofApp::setupViewports(){
    panelJointsIK->setWidth(ofGetWindowWidth()/24*3);
    panelJoints->setWidth(ofGetWindowWidth()/24*2);
    panel->setWidth(ofGetWindowWidth()/24*2);

    viewportReal = ofRectangle((21*ofGetWindowWidth()/24)/2, 0,  (21*ofGetWindowWidth()/24)/2, 6*ofGetWindowHeight()/8);
    viewportSim = ofRectangle(0, 0, (21*ofGetWindowWidth()/24)/2-panel->getWidth(), 3*ofGetWindowHeight()/8);

    float simWidth = (21*ofGetWindowWidth()/24)/2;
    
    viewportSimP0 = ofRectangle(0, viewportSim.height, simWidth/3, viewportSim.width/3);
    viewportSimP1 = ofRectangle(simWidth/3, viewportSim.height, simWidth/3, viewportSim.width/3);
    viewportSimP2 = ofRectangle(2*simWidth/3, viewportSim.height, simWidth/3,viewportSim.width/3);
    
    viewportRealP0 = ofRectangle(viewportReal.x, viewportReal.height, viewportReal.width/3, viewportReal.width/3);
    viewportRealP1 = ofRectangle(viewportReal.x+viewportReal.width/3, viewportReal.height, viewportReal.width/3, viewportReal.width/3);
    viewportRealP2 = ofRectangle(viewportReal.x+2*viewportReal.width/3, viewportReal.height, viewportReal.width/3, viewportReal.width/3);
    
    activeCam = real;
    
    gizmo.setViewDimensions(viewportSim.width, viewportSim.height);
    
    for(int i = 0; i < N_CAMERAS; i++){
        cams.push_back(new ofEasyCam());
        savedCamMats.push_back(ofMatrix4x4());
        viewportLabels.push_back("");
    }
    
    
    cams[real]->setPosition(glm::vec3(-1, 1, 2500));
    cams[real]->lookAt(glm::vec3(0, 0, 0), camUp);
    
    
    cams[sim]->setPosition(glm::vec3(-1, 1, 2500));
    cams[sim]->lookAt(glm::vec3(0, 0, 0), camUp);
    
    cams[real]->begin(viewportReal);
    robot.draw();
    cams[real]->end();
    cams[real]->enableMouseInput();
    
    
    cams[sim]->begin(viewportSim);
    robot.draw();
    cams[sim]->end();
    cams[sim]->enableMouseInput();
    
    updateActiveCamera();
    
}

//--------------------------------------------------------------
void ofApp::setupGUI(){
    
    panel = gui.addPanel(parameters.robotArmParams);
 
    panel->loadTheme("theme3.json", true);
  
    panelJoints = gui.addPanel(parameters.safety);
    panelJoints->loadTheme("theme3.json", true);

    panelJointsIK = gui.addPanel(parameters.targetJoints);
    panelJointsIK->add(parameters.joints);
    panelJointsIK->loadTheme("theme3.json", true);

    parameters.bMove = false;
    // get the current pose on start up
    panel->loadFromFile("settings/settings.xml");
    
    
    timeline.setup();
    timeline.setLoopType(OF_LOOP_NORMAL);
    timeline.setDurationInSeconds(500);
    //each call to "add keyframes" add's another track to the timeline
    
    jointNames.push_back("Joint0");
    jointNames.push_back("Joint1");
    jointNames.push_back("Joint2");
    jointNames.push_back("Joint3");
    jointNames.push_back("Joint4");
    jointNames.push_back("Joint5");
    
    curves.push_back(timeline.addCurves(jointNames[0], ofRange(-TWO_PI, TWO_PI)));
    curves.push_back(timeline.addCurves(jointNames[1], ofRange(-TWO_PI, TWO_PI)));
    curves.push_back(timeline.addCurves(jointNames[2], ofRange(-TWO_PI, TWO_PI)));
    curves.push_back(timeline.addCurves(jointNames[3], ofRange(-TWO_PI, TWO_PI)));
    curves.push_back(timeline.addCurves(jointNames[4], ofRange(-TWO_PI, TWO_PI)));
    curves.push_back(timeline.addCurves(jointNames[5], ofRange(-TWO_PI, TWO_PI)));
    
    // setup Gizmo
    
    tcpNode.setPosition(ofVec3f(200, 200, 200));
    tcpNode.setOrientation(parameters.targetTCP.rotation);
    gizmo.setNode(tcpNode);
    gizmo.setDisplayScale(1.0);
    gizmo.show();
    
    moveColor = ofFloatColor(0, 1, 0, 1);
    freedriveColor = ofFloatColor(0, 0, 1, 1);
    stopColor = ofFloatColor(1, 0, 0, 1);
    
    previewMode = stopColor;
    realMode = stopColor;
}

void ofApp::addRealPoseKeyFrame(){
    auto pose = robot.getCurrentPose();
    
    for(int i = 0 ; i < curves.size(); i++){
        curves[i]->addKeyframe((pose[i]));
    }
}

void ofApp::positionGUI(){

    panel->setPosition(viewportSim.x+viewportSim.width, 0);
    panelJointsIK->setPosition(viewportReal.x+viewportReal.width, 0);
    panelJoints->setPosition(viewportReal.x+viewportReal.width, panelJointsIK->getHeight());
    
    timeline.setWidth(viewportReal.width);
    timeline.setHeight(2*ofGetWindowHeight()/8);
    timeline.setOffset(ofVec2f(0, viewportSimP0.y+viewportSimP0.height));
    
}

//--------------------------------------------------------------
void ofApp::drawGUI(){
    
    
    camP0.begin(viewportSimP0);
    ofDrawAxis(100);
    tcpNode.draw();
    robot.drawPreview(previewMode);
    camP0.end();
    
    camP1.begin(viewportSimP1);
    ofDrawAxis(100);
    tcpNode.draw();
    robot.drawPreview(previewMode);
    camP1.end();
    
    camP2.begin(viewportSimP2);
    ofDrawAxis(100);
    tcpNode.draw();
    robot.drawPreview(previewMode);
    camP2.end();
    
    camP0.begin(viewportRealP0);
    ofDrawAxis(100);
    tcpNode.draw();
    robot.draw(realMode);
    camP0.end();
    
    camP1.begin(viewportRealP1);
    ofDrawAxis(100);
    tcpNode.draw();
    robot.draw(realMode);
    camP1.end();
    
    camP2.begin(viewportRealP2);
    ofDrawAxis(100);
    tcpNode.draw();
    robot.draw(realMode);
    camP2.end();
    
    
    timeline.draw();
    
    
    hightlightViewports();
}



//--------------------------------------------------------------
void ofApp::updateActiveCamera(){
    
    if (viewportReal.inside(ofGetMouseX(), ofGetMouseY()))
    {
        activeCam = real;
        if(!cams[real]->getMouseInputEnabled()){
            cams[real]->enableMouseInput();
        }
        if(cams[sim]->getMouseInputEnabled()){
            cams[sim]->disableMouseInput();
        }
    }else{
        if(cams[real]->getMouseInputEnabled()){
            cams[real]->disableMouseInput();
        }
    }
    if(viewportSim.inside(ofGetMouseX(), ofGetMouseY()))
    {
        activeCam = sim;
        if(!cams[sim]->getMouseInputEnabled()){
            cams[sim]->enableMouseInput();
        }
        if(cams[real]->getMouseInputEnabled()){
            cams[real]->disableMouseInput();
        }
        if(gizmo.isInteracting() && cams[sim]->getMouseInputEnabled()){
            cams[sim]->disableMouseInput();
        }
    }else{
        if(cams[sim]->getMouseInputEnabled()){
            cams[sim]->disableMouseInput();
        }
    }
}

//--------------------------------------------------------------
//--------------------------------------------------------------
void ofApp::handleViewportPresets(int key){
    
    float dist = 1500;
    float zOffset = 450;
    
    camP0.setPosition(glm::vec3(dist, 1, 1));
    camP1.setPosition(glm::vec3(1, dist, 1));
    camP2.setPosition(glm::vec3(dist/4, dist, dist/4));
    camP0.lookAt(glm::vec3(0, 0, 0), camUp);
    camP1.lookAt(glm::vec3(0, 0, 0), camUp);
    camP2.lookAt(glm::vec3(0, 0, 0), camUp);
    
    if(activeCam != -1){
        glm::vec3 pos = lookAtTCP.get()==true?glm::vec3(0, 0, 0):tcpNode.getPosition();
        // TOP VIEW
        if (key == '1'){
            glm::vec3 offset = glm::vec3(-1, 1, dist);
            cams[activeCam]->reset();
            cams[activeCam]->setPosition(pos+offset);
            cams[activeCam]->lookAt(lookAtTCP.get()==true?tcpNode.getPosition():glm::vec3(0, 0, 0), camUp);
            viewportLabels[activeCam] = "TOP VIEW";
        
            return;
        }
        // LEFT VIEW
        if (key == '2'){
            glm::vec3 offset = glm::vec3(dist, 1, 1);
            cams[activeCam]->reset();
            cams[activeCam]->setPosition(pos+offset);
            cams[activeCam]->lookAt(lookAtTCP.get()==true?tcpNode.getPosition():glm::vec3(0, 0, 0), camUp);
            viewportLabels[activeCam] = "LEFT VIEW";
            

            
            return;
        }
        // FRONT VIEW
        if (key == '3'){
            glm::vec3 offset = glm::vec3(1, dist, 1);
            cams[activeCam]->reset();
            cams[activeCam]->setPosition(pos+offset);
            cams[activeCam]->lookAt(lookAtTCP.get()==true?tcpNode.getPosition():glm::vec3(0, 0, 0), camUp);
            viewportLabels[activeCam] = "FRONT VIEW";
            

            
            return;
        }
        // PERSPECTIVE VIEW
        if (key == '4'){
            glm::vec3 offset = glm::vec3(dist/4, dist, dist/4);
            cams[activeCam]->reset();
            cams[activeCam]->setPosition(pos+offset);
            cams[activeCam]->lookAt(lookAtTCP.get()==true?tcpNode.getPosition():glm::vec3(0, 0, 0), camUp);
            viewportLabels[activeCam] = "PERSPECTIVE VIEW";
            
    
            return;
        }
    }
    
    

}


//--------------------------------------------------------------
void ofApp::hightlightViewports(){
    ofPushStyle();
    
    // highlight right viewport
    if (activeCam == real){
        
        ofSetLineWidth(6);
        ofSetColor(ofColor::white,0.1);
        ofDrawRectangle(viewportReal);
        
    }
    // hightlight left viewport
    else{
        ofSetLineWidth(6);
        ofSetColor(ofColor::white,0.1);
        ofDrawRectangle(viewportSim);
    }
    
    // show Viewport info
    ofSetColor(ofColor::white,200);
    ofDrawBitmapString(viewportLabels[0], viewportReal.x+viewportReal.width-120, ofGetWindowHeight()-30);
    ofDrawBitmapString("REALTIME", ofGetWindowWidth()/2 - 90, ofGetWindowHeight()-30);
    ofDrawBitmapString(viewportLabels[1], viewportSim.x+viewportSim.width-120, ofGetWindowHeight()-30);
    ofDrawBitmapString("SIMULATED", 30, ofGetWindowHeight()-30);
    
    ofPopStyle();
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
    if(key == 'm'){
        parameters.bMove = !parameters.bMove;
    }
    if( key == 'r' ) {
        gizmo.setType( ofxGizmo::OFX_GIZMO_ROTATE );
    }
    if( key == 'g' ) {
        gizmo.setType( ofxGizmo::OFX_GIZMO_MOVE );
    }
    if( key == 's' ) {
        gizmo.setType( ofxGizmo::OFX_GIZMO_SCALE );
    }
    if( key == 'e' ) {
        gizmo.toggleVisible();
    }
    if(key == '='){
        addRealPoseKeyFrame();
    }
    
    handleViewportPresets(key);
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
    
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    viewportLabels[activeCam] = "";
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){
    
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){
    
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    setupViewports();
    positionGUI();
    
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
    
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 
    
}
