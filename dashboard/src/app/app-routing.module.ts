import { Page1Component } from './page1/page1.component';
import { Page2Component } from './page2/page2.component';
import {PageRootComponent} from './page-root/page-root.component'
import { PageNotFoudComponent } from './page-not-foud/page-not-foud.component';
import { NgModule } from '@angular/core';
import { Routes, Route, RouterModule } from '@angular/router';


// *********************
// Custome Routes
// *********************
const routes: Routes = [
  { path: '', redirectTo: '/home', pathMatch: 'full' },
  {path: 'home', component: PageRootComponent},
  {path: 'page-1', component: Page1Component},
  {path: 'page-2', component: Page2Component},
  { path: '**', pathMatch: 'full', component: PageNotFoudComponent },
];

@NgModule({
  imports: [RouterModule.forRoot(routes)],
  exports: [RouterModule]
})
export class AppRoutingModule { }
