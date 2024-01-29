import { Component, HostListener, ElementRef, ViewChild, Input } from '@angular/core';
import { MatDialogRef } from '@angular/material/dialog';
import { FormControl, FormGroup, Validators} from '@angular/forms';
import { DbCoreService } from '../services/db-core.service';

@Component({
  selector: 'app-dialog-logon',
  templateUrl: './dialog-logon.component.html',
  styleUrls: ['./dialog-logon.component.css', '../app.component.css']
  // providers: [DbCoreService]
})
export class DialogLogonComponent{
  Icon: string;
  Spiner: string;
  Interval;


  @ViewChild('in_user') tuser: ElementRef;
  @ViewChild('in_pwd')  tpwd: ElementRef;
  @ViewChild('in_host') thost: ElementRef;
  @ViewChild('in_port') tport: ElementRef;

  @ViewChild('lgn_btn_ok') button: ElementRef;
  @ViewChild('img_spiner') img: ElementRef;
  @ViewChild('fb_error') lberror: ElementRef;

  loginForm = new FormGroup({
    user: new FormControl(''),
    pwd: new FormControl(''),
    host: new FormControl(''),
    port: new FormControl('', Validators.pattern('[0-9]{3,9}'))
  });

  constructor(  private core: DbCoreService,
                private dialogRef: MatDialogRef<DialogLogonComponent>,
                private elementsRef: ElementRef
              ) {
    dialogRef.disableClose = true;
    this.Icon = '/assets/img/login.png';
    this.Spiner = '/assets/gif/spin_200px.gif';
  }

  showControls(bShow: boolean): void
  {
    if (bShow)
    {
      this.tuser.nativeElement.disabled = false;
      this.tpwd.nativeElement.disabled = false;
      this.thost.nativeElement.disabled = false;
      this.tport.nativeElement.disabled = false;

      this.button.nativeElement.hidden = false;
      this.img.nativeElement.hidden = true;

      clearInterval(this.Interval);
      this.dialogRef.close();
    }
    else
    {
      this.tuser.nativeElement.disabled = true;
      this.tpwd.nativeElement.disabled = true;
      this.thost.nativeElement.disabled = true;
      this.tport.nativeElement.disabled = true;

      this.button.nativeElement.hidden = true;
      this.img.nativeElement.hidden = false;

      this.Interval = setInterval(() => {
        this.showControls(true);
      }, 5000);
    }
  }

  setErrorMessage(msg: string): void{
    this.lberror.nativeElement.innerText = msg;
  }

  @HostListener('window:keyup.enter') onKeyUp(){
    if ( this.button.nativeElement.hidden === false )
    {
      this.onSubmit();
    }
  }

  onSubmit(): void
  {

    console.log('onSubmit');
    console.log(this.loginForm.value);
    console.log(this.loginForm.valid);

    const rc = this.core.Login(
      {
        user: this.tuser.nativeElement.value,
        pwd: this.tpwd.nativeElement.value,
        host: this.thost.nativeElement.value,
        port: this.tport.nativeElement.value,
      }
    );

    console.log('Login Core: ' + rc.code);
    console.log('Login Desc: ' + rc.desc);

    this.showControls(false);
  }


}
