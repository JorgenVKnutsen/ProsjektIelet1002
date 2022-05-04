#include "Steer.h"

/*
Her settes kun verdien til rotGain, som er en viktig variabel
for å få Zumoen til å svinge passe mye if avstand fra midten av
linja
*/
Steer::Steer(){
    rotGain = desLinSpeed*0.00105;
}

/*
Funksjonen som returnerer hastigheten venstre hjul skal ha ift. posisjon på linja
*/
Steer::setLeft(int linePos, int desLinSpeed){
  
    rotSpeed = rotGain*(2000-linePos);
    
    float leftSpeed = desLinSpeed - rotSpeed;
    float rightSpeed = desLinSpeed + rotSpeed;

    if(leftSpeed > 400){
        leftSpeed = 400;
    }else if(rightSpeed > 400){
        leftSpeed = leftSpeed - (rightSpeed-400);
        if(rightSpeed < -400){
            leftSpeed = 400;
        }
    }else if(leftSpeed < -400){
        leftSpeed = -400;
    }else if(rightSpeed < -400){
        leftSpeed = leftSpeed - (rightSpeed + 400);
    }
    
    return leftSpeed;
}

/*
Returnerer hastigheten Zumoen skal ha på høyre hjul
*/
Steer::setRight(int linePos, int desLinSpeed){
  
    rotSpeed = rotGain*(2000-linePos);
    
    float leftSpeed = desLinSpeed - rotSpeed;
    float rightSpeed = desLinSpeed + rotSpeed;

    if(leftSpeed > 400){
        rightSpeed = rightSpeed - (leftSpeed-400);
        if(rightSpeed < -400){
            rightSpeed = 400;
        }
    }else if(rightSpeed > 400){
        rightSpeed= 400;
    }else if(leftSpeed < -400){
        rightSpeed = rightSpeed - (leftSpeed + 400);
        if(rightSpeed > 400){
            rightSpeed = 400;
        }
    }else if(rightSpeed < -400){
        rightSpeed = -400;
    }
    
    return rightSpeed;
}
/* 
Styrekoden er sterkt inspirert fra  : https://github.com/kdhansen/zumo-line-follower
returnerer rightSpeed og leftSpeed i to separate funksjoner fordi å returnere 2 verdier samtidig
viste seg å være vanskelig
*/

  
