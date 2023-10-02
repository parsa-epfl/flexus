// Simple benchmark to verify branches rates

bool toggle = true;
int x = 0, y = 0;

void outerloop(){
    y++;
    if(y%2 == 0){
        x++;
    }
    else{
        innerloop();
    }
    return;
}

void innerloop(){
    x++;
    return;
}

int main(){
    while(1){
        outerloop();
    }
    return 0;
}