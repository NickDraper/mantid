
#include <iostream>
#include  "MantidQtImageViewer/ImageView.h"
#include  "MantidQtImageViewer/ColorMaps.h"

#include "ui_ImageView.h"
#include "MantidQtImageViewer/IVConnections.h"
#include "MantidQtImageViewer/ImageDisplay.h"
#include "MantidQtImageViewer/SliderHandler.h"
#include "MantidQtImageViewer/RangeHandler.h"
#include "MantidQtImageViewer/EModeHandler.h"
#include "MantidQtImageViewer/MatrixWSDataSource.h"

namespace MantidQt
{
namespace ImageView
{


/**
 *  Construct an ImageView to display data from the specified data source.
 *  The specified ImageDataSource must be constructed elsewhere and passed
 *  into this ImageView constructor.  Most other components of the ImageView
 *  are managed by this class.  That is the graphs, image display and other
 *  parts of the ImageView are constructed here and are deleted when the
 *  ImageView destructor is called.
 *
 *  @param data_source  The source of the data that will be displayed. 
 */
ImageView::ImageView( ImageDataSource* data_source )
{
  Ui_ImageViewer* ui = new Ui_ImageViewer();
  saved_ui          = ui; 

                                          // IF we have a MatrixWSDataSource
                                          // give it the handler for the
                                          // EMode, so the user can set EMode
                                          // and EFixed.  NOTE: we could avoid
                                          // this type checking if we made the
                                          // ui in the calling code and passed
                                          // it in.  We would need a common
                                          // base class for this class and
                                          // the ref-viewer UI.
  MatrixWSDataSource* matrix_ws_data_source = 
                      dynamic_cast<MatrixWSDataSource*>( data_source );
  if ( matrix_ws_data_source != 0 )
  {
    EModeHandler* emode_handler = new EModeHandler( ui );
    saved_emode_handler = emode_handler;
    matrix_ws_data_source -> SetEModeHandler( emode_handler );
  }
  else
  {
    saved_emode_handler = 0;
  }

  QMainWindow* window = this;

  ui->setupUi( window );
  window->resize( 1050, 800 );
  window->show();
  window->setAttribute(Qt::WA_DeleteOnClose);  // We just need to close the
                                               // window to trigger the 
                                               // destructor and clean up

  SliderHandler* slider_handler = new SliderHandler( ui );
  saved_slider_handler = slider_handler;

  RangeHandler* range_handler = new RangeHandler( ui );
  saved_range_handler = range_handler;

  h_graph = new GraphDisplay( ui->h_graphPlot, ui->h_graph_table, false );
  v_graph = new GraphDisplay( ui->v_graphPlot, ui->v_graph_table, true );

  ImageDisplay* image_display = new ImageDisplay( ui->imagePlot,
                                                  slider_handler,
                                                  range_handler,
                                                  h_graph, v_graph,
                                                  ui->image_table );
  saved_image_display = image_display;

  IVConnections* iv_connections = new IVConnections( ui, this, 
                                                     image_display, 
                                                     h_graph, v_graph );
  saved_iv_connections = iv_connections;

  image_display->SetDataSource( data_source );
}


ImageView::~ImageView()
{
//  std::cout << "ImageView destructor called" << std::endl;

  delete  h_graph;
  delete  v_graph;

  ImageDisplay* image_display = static_cast<ImageDisplay*>(saved_image_display);
  delete  image_display;

  SliderHandler* slider_handler = 
                             static_cast<SliderHandler*>(saved_slider_handler);
  delete  slider_handler;

  RangeHandler* range_handler = 
                             static_cast<RangeHandler*>(saved_range_handler);
  delete  range_handler;

  IVConnections* iv_connections = 
                             static_cast<IVConnections*>(saved_iv_connections);
  delete  iv_connections;

  Ui_ImageViewer* ui = static_cast<Ui_ImageViewer*>(saved_ui);
  delete  ui;

  if ( saved_emode_handler != 0 )
  {
    EModeHandler* emode_handler = 
                             static_cast<EModeHandler*>(saved_emode_handler);
    delete emode_handler;
  }
}


} // namespace MantidQt 
} // namespace ImageView 

