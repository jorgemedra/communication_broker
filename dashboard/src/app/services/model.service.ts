import { Injectable } from '@angular/core';


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
//#region Beans

export interface LoginData
{
  user: string;
  pwd: string;
  host: string;
  port: string;
}

export enum SesionType
{
  CPP = 1,
  JS,
  Python,
  CSharp,

}

export interface SessionData
{
  user_id: string;
  type_id: SesionType;
  remote_host: string;
  remote_port: number;
}

//#endregion
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>


@Injectable({
  providedIn: 'root'
})
export class ModelService {
  login: LoginData;

  constructor( ) {

    this.login = {user: '', pwd: '', host: '', port: ''};
  }


}
