import { AfterViewInit, Component, ViewChild } from '@angular/core';
import { MatDialog} from '@angular/material/dialog';
import { ActivatedRoute, NavigationEnd, Router } from '@angular/router';
import { DialogYesnoComponent } from './dialog-yesno/dialog-yesno.component';
import { DialogLogonComponent } from './dialog-logon/dialog-logon.component';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css'],
})

export class AppComponent implements AfterViewInit{
  title = 'Communication Broker Dashboard';
  version = '1.0.0';
  mlabel = 'This is my data';

  constructor(
      public dialog: MatDialog,
      private route: ActivatedRoute,
      private router: Router){


  }

  ngAfterViewInit() {
    this.login_session();

    this.router.events.subscribe(val => {
      if (val instanceof NavigationEnd) {
        console.log('this.router.events.subscribe: url: ' + val.url);
        console.log('this.router.events.subscribe: id: ' + val.id);
        console.log('this.router.events.subscribe: urlAfterRedirects: ' + val.urlAfterRedirects);
      }
    });
  }

  onPushme(): void
  {
    const dialogRef = this.dialog.open(DialogYesnoComponent, {data: {text: "This is my question apara que aa adfagsdgw sfgs dfgsd sf s fsf d  dadada" }} );

    dialogRef.afterClosed().subscribe(result => {
      if ( result === true )
      {
        // alert('Was succesful');
      }
      else
      {
        // alert('Was canceled');
      }
    });
  }

  login_session(): void
  {
    const dialogRef = this.dialog.open(DialogLogonComponent);

    dialogRef.afterClosed().subscribe(result => {
      if ( result === true )
      {
        // console.log('DialogLogonComponent = ' + result);
      }
      else
      {
        // console.log('DialogLogonComponent = ' + result);
      }
    });
  }

}
