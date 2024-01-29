import { Component, OnInit } from '@angular/core';

@Component({
  selector: 'app-login-form',
  templateUrl: './login-form.component.html',
  styleUrls: ['./login-form.component.css', '../app.component.css']
})
export class LoginFormComponent implements OnInit {

  icon = '../../img/favicon.ico';
  title = 'Login';
  visible = true;

  constructor() { }

  ngOnInit(): void {
  }

  onClickLogin(): void
  {
    this.visible = false;
    return;
  }

}
