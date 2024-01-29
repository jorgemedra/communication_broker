import { LoginData, ModelService } from './model.service';
import { Injectable } from '@angular/core';

export interface BasicResult{
  code: number;
  desc: string;
}

declare var MyClass;

@Injectable({
  providedIn: 'root'
})
export class DbCoreService {

  counter: number;
  login: LoginData;
  mc: any;

  constructor(
      private model: ModelService
              ) {
    console.log('DbCoreService has been created');
    this.counter = 0;
    this.mc = new MyClass();
  }

  Login_CB(message: any): void{
    console.log('Login_CB: ' + message);
  }

  Login(data: LoginData): BasicResult
  {
    console.log('----- LOGIN -------');
    console.log(' usr:  [' + data.user + ']');
    console.log(' pwd:  [' + data.pwd + ']');
    console.log(' host: [' + data.host + ']');
    console.log(' port: [' + data.port + ']');

    this.counter++;

    this.model.login.user = data.user;
    this.model.login.pwd = ''; // The PWD is hidding.
    this.model.login.host = data.host;
    this.model.login.port = data.port;

    this.mc.setCBF(this.Login_CB);
    this.mc.start();

    return {code: 1, desc: 'This is my basic error message to show how it works. Counter = ' + this.counter};
  }

}
